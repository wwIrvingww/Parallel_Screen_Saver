#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE 4096
#define MAX_STR   64

typedef struct {
    char exec[MAX_STR];      // "seq" | "omp"
    char mode[MAX_STR];      // "rain" | "bounce" | "spiral" | "nebula"
    int  threads_req;        // solicitado por CLI
    int  threads_eff;        // efectivo (omp_get_max_threads)
    int  width, height, N, frame;
    double speed, dt_s, update_ms, render_ms, total_ms, fps;
} Row;

typedef struct {
    char exec[MAX_STR];
    char mode[MAX_STR];
    int  width, height, N;
    double speed;
    int  threads_req;

    // acumuladores
    double sum_update, sum_render, sum_total, sum_fps;
    double sum_threads_eff;
    int    cnt_threads_eff;
    int    frames;
} Group;

static char* trim(char* s) {
    if (!s) return s;
    char* p = s; while (*p && isspace((unsigned char)*p)) ++p;
    if (p != s) memmove(s, p, strlen(p) + 1);
    size_t n = strlen(s);
    while (n && isspace((unsigned char)s[n-1])) s[--n] = '\0';
    return s;
}

static int split_csv(char* line, char* out[], int max_tokens) {
    // CSV simple sin comillas: separa por coma
    int count = 0;
    char* p = line;
    while (count < max_tokens) {
        out[count++] = p;
        char* c = strchr(p, ',');
        if (!c) break;
        *c = '\0';
        p = c + 1;
    }
    return count;
}

static int parse_row_from_tokens(Row* r, char* toks[], int n) {
    // Espera 14 columnas:
    // 0 exec,1 mode,2 threads_req,3 threads_eff,4 width,5 height,6 N,7 speed,
    // 8 frame,9 dt_s,10 update_ms,11 render_ms,12 total_ms,13 fps
    if (n < 14) return 0;
    strncpy(r->exec, trim(toks[0]), MAX_STR-1);
    strncpy(r->mode, trim(toks[1]), MAX_STR-1);
    r->threads_req = atoi(trim(toks[2]));
    r->threads_eff = atoi(trim(toks[3]));
    r->width  = atoi(trim(toks[4]));
    r->height = atoi(trim(toks[5]));
    r->N      = atoi(trim(toks[6]));
    r->speed  = atof(trim(toks[7]));
    r->frame  = atoi(trim(toks[8]));
    r->dt_s   = atof(trim(toks[9]));
    r->update_ms = atof(trim(toks[10]));
    r->render_ms = atof(trim(toks[11]));
    r->total_ms  = atof(trim(toks[12]));
    r->fps       = atof(trim(toks[13]));
    return 1;
}

static int same_key(const Group* g, const Row* r) {
    return strcmp(g->exec, r->exec) == 0 &&
           strcmp(g->mode, r->mode) == 0 &&
           g->width  == r->width &&
           g->height == r->height &&
           g->N      == r->N &&
           g->threads_req == r->threads_req &&
           g->speed == r->speed; // asumimos entrada consistente
}

static void add_row_to_group(Group* g, const Row* r) {
    g->sum_update += r->update_ms;
    g->sum_render += r->render_ms;
    g->sum_total  += r->total_ms;
    g->sum_fps    += r->fps;
    if (r->threads_eff > 0) { g->sum_threads_eff += r->threads_eff; g->cnt_threads_eff++; }
    g->frames++;
}

static void init_group_from_row(Group* g, const Row* r) {
    memset(g, 0, sizeof(*g));
    strncpy(g->exec, r->exec, MAX_STR-1);
    strncpy(g->mode, r->mode, MAX_STR-1);
    g->width  = r->width;
    g->height = r->height;
    g->N      = r->N;
    g->speed  = r->speed;
    g->threads_req = r->threads_req;
    add_row_to_group(g, r);
}

typedef struct {
    Group* data;
    int    size;
    int    cap;
} GroupVec;

static void gv_init(GroupVec* v) { v->data=NULL; v->size=0; v->cap=0; }
static void gv_free(GroupVec* v) { free(v->data); v->data=NULL; v->size=v->cap=0; }

static Group* gv_find(GroupVec* v, const Row* r) {
    for (int i=0;i<v->size;i++) if (same_key(&v->data[i], r)) return &v->data[i];
    return NULL;
}

static Group* gv_add(GroupVec* v, const Row* r) {
    if (v->size == v->cap) {
        int ncap = v->cap ? v->cap*2 : 64;
        Group* nd = (Group*)realloc(v->data, ncap * sizeof(Group));
        if (!nd) return NULL;
        v->data = nd; v->cap = ncap;
    }
    init_group_from_row(&v->data[v->size], r);
    return &v->data[v->size++];
}

static int load_rows(const char* path, Row** out_rows, int* out_nrows) {
    FILE* f = fopen(path, "r");
    if (!f) { fprintf(stderr, "No pude abrir %s\n", path); return 0; }

    char line[MAX_LINE];
    // header
    if (!fgets(line, sizeof(line), f)) { fclose(f); return 0; }

    // validar columnas mínimas
    // Permitimos encabezado con "exec,"; si no, aborta.
    if (strncmp(line, "exec,", 5) != 0) {
        fprintf(stderr, "Encabezado inesperado. Esperaba columnas que inicien con 'exec,'.\n");
        fclose(f);
        return 0;
    }

    int cap = 1024, n = 0;
    Row* rows = (Row*)malloc(cap * sizeof(Row));
    if (!rows) { fclose(f); return 0; }

    while (fgets(line, sizeof(line), f)) {
        char* p = strchr(line, '\n'); if (p) *p = '\0';
        if (!*line) continue;
        char* toks[32]; int nt = split_csv(line, toks, 32);
        Row r;
        if (!parse_row_from_tokens(&r, toks, nt)) continue;

        if (n == cap) {
            cap *= 2;
            Row* nr = (Row*)realloc(rows, cap * sizeof(Row));
            if (!nr) { free(rows); fclose(f); return 0; }
            rows = nr;
        }
        rows[n++] = r;
    }
    fclose(f);
    *out_rows = rows;
    *out_nrows = n;
    return 1;
}

static void dirname_from_path(const char* in, char* out, size_t outsz) {
    strncpy(out, in, outsz-1); out[outsz-1]='\0';
    char* slash = strrchr(out, '/');
#ifdef _WIN32
    char* bslash = strrchr(out, '\\');
    if (!slash || (bslash && bslash > slash)) slash = bslash;
#endif
    if (slash) *slash = '\0';
    else { out[0]='.'; out[1]='\0'; }
}

static int write_summary_csv(const char* dir, const GroupVec* v, const GroupVec* bases) {
    char outpath[1024];
    snprintf(outpath, sizeof(outpath), "%s/bench_summary2.csv", dir);
    FILE* f = fopen(outpath, "w");
    if (!f) { fprintf(stderr, "No pude escribir %s\n", outpath); return 0; }

    fprintf(f, "exec,mode,threads_req,threads_eff_avg,width,height,N,speed,frames,update_ms_avg,render_ms_avg,total_ms_avg,fps_avg,speedup_vs_seq\n");
    for (int i=0;i<v->size;i++) {
        const Group* g = &v->data[i];
        double up = g->frames ? g->sum_update / g->frames : 0.0;
        double rp = g->frames ? g->sum_render / g->frames : 0.0;
        double tp = g->frames ? g->sum_total  / g->frames : 0.0;
        double fp = g->frames ? g->sum_fps    / g->frames : 0.0;
        double th_eff = g->cnt_threads_eff ? g->sum_threads_eff / g->cnt_threads_eff : 0.0;

        // buscar baseline secuencial (mismo mode,width,height,N,speed)
        double speedup = 0.0;
        for (int j=0;j<bases->size;j++) {
            const Group* b = &bases->data[j];
            if (strcmp(b->mode, g->mode)==0 &&
                b->width==g->width && b->height==g->height &&
                b->N==g->N && b->speed==g->speed) {
                double t1 = b->frames ? b->sum_total / b->frames : 0.0;
                if (tp > 0.0 && t1 > 0.0) speedup = t1 / tp;
                break;
            }
        }

        fprintf(f, "%s,%s,%d,%.2f,%d,%d,%d,%.3f,%d,%.3f,%.3f,%.3f,%.2f,%.3f\n",
            g->exec, g->mode, g->threads_req, th_eff, g->width, g->height, g->N, g->speed,
            g->frames, up, rp, tp, fp, speedup);
    }
    fclose(f);
    printf("Escrito: %s\n", outpath);
    return 1;
}

static int write_anexo_md(const char* dir, const GroupVec* v, const GroupVec* bases) {
    char outpath[1024];
    snprintf(outpath, sizeof(outpath), "%s/Anexo3_Bitacora2.md", dir);
    FILE* f = fopen(outpath, "w");
    if (!f) { fprintf(stderr, "No pude escribir %s\n", outpath); return 0; }

    fprintf(f, "# Anexo 3 — Bitácora de pruebas\n\n");
    fprintf(f, "Resumen de ejecuciones (sec./par.) agregadas desde el CSV de benchmark.\n\n");
    fprintf(f, "| exec | mode | threads_req | threads_eff_avg | width | height | N | speed | frames | update_ms_avg | render_ms_avg | total_ms_avg | fps_avg | speedup_vs_seq |\n");
    fprintf(f, "|:----:|:----:|:-----------:|:---------------:|:-----:|:------:|:--:|:-----:|:------:|--------------:|--------------:|-------------:|--------:|---------------:|\n");

    for (int i=0;i<v->size;i++) {
        const Group* g = &v->data[i];
        double up = g->frames ? g->sum_update / g->frames : 0.0;
        double rp = g->frames ? g->sum_render / g->frames : 0.0;
        double tp = g->frames ? g->sum_total  / g->frames : 0.0;
        double fp = g->frames ? g->sum_fps    / g->frames : 0.0;
        double th_eff = g->cnt_threads_eff ? g->sum_threads_eff / g->cnt_threads_eff : 0.0;

        double speedup = 0.0;
        for (int j=0;j<bases->size;j++) {
            const Group* b = &bases->data[j];
            if (strcmp(b->mode, g->mode)==0 &&
                b->width==g->width && b->height==g->height &&
                b->N==g->N && b->speed==g->speed) {
                double t1 = b->frames ? b->sum_total / b->frames : 0.0;
                if (tp > 0.0 && t1 > 0.0) speedup = t1 / tp;
                break;
            }
        }

        fprintf(f, "| %s | %s | %d | %.2f | %d | %d | %d | %.3f | %d | %.3f | %.3f | %.3f | %.2f | %.3f |\n",
            g->exec, g->mode, g->threads_req, th_eff, g->width, g->height, g->N, g->speed,
            g->frames, up, rp, tp, fp, speedup);
    }
    fprintf(f, "\n");
    fclose(f);
    printf("Escrito: %s\n", outpath);
    return 1;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s bench/bench_YYYYmmdd_HHMMSS.csv\n", argv[0]);
        return 1;
    }

    Row* rows = NULL; int nrows = 0;
    if (!load_rows(argv[1], &rows, &nrows)) {
        return 1;
    }
    if (nrows == 0) {
        fprintf(stderr, "CSV sin filas.\n");
        free(rows);
        return 1;
    }

    GroupVec groups; gv_init(&groups);
    for (int i=0;i<nrows;i++) {
        Group* g = gv_find(&groups, &rows[i]);
        if (!g) g = gv_add(&groups, &rows[i]);
        if (!g) { fprintf(stderr, "Memoria insuficiente.\n"); free(rows); gv_free(&groups); return 1; }
        add_row_to_group(g, &rows[i]);
    }

    // Baselines secuenciales por (mode,width,height,N,speed)
    GroupVec bases; gv_init(&bases);
    for (int i=0;i<groups.size;i++) {
        Group* g = &groups.data[i];
        if (strcmp(g->exec, "seq")==0) {
            // clave sólo por configuración sin threads_req
            // guardamos una copia
            Group copy = *g;
            // threads_req no importa para el baseline
            Group* slot = NULL;
            // buscar si ya existe exacto (misma conf)
            for (int j=0;j<bases.size;j++) {
                Group* b = &bases.data[j];
                if (strcmp(b->mode, g->mode)==0 &&
                    b->width==g->width && b->height==g->height &&
                    b->N==g->N && b->speed==g->speed) {
                    slot = b; break;
                }
            }
            if (!slot) {
                if (bases.size == bases.cap) {
                    int ncap = bases.cap ? bases.cap*2 : 32;
                    Group* nd = (Group*)realloc(bases.data, ncap*sizeof(Group));
                    if (!nd) { fprintf(stderr, "Memoria insuficiente.\n"); free(rows); gv_free(&groups); gv_free(&bases); return 1; }
                    bases.data = nd; bases.cap = ncap;
                }
                bases.data[bases.size++] = copy;
            } else {
                // acumula si hubiera múltiples grupos seq (distinto threads_req)
                slot->sum_update += g->sum_update;
                slot->sum_render += g->sum_render;
                slot->sum_total  += g->sum_total;
                slot->sum_fps    += g->sum_fps;
                slot->sum_threads_eff += g->sum_threads_eff;
                slot->cnt_threads_eff += g->cnt_threads_eff;
                slot->frames     += g->frames;
            }
        }
    }

    char outdir[1024];
    dirname_from_path(argv[1], outdir, sizeof(outdir));

    int ok1 = write_summary_csv(outdir, &groups, &bases);
    int ok2 = write_anexo_md(outdir, &groups, &bases);

    free(rows);
    gv_free(&groups);
    gv_free(&bases);
    return (ok1 && ok2) ? 0 : 1;
}