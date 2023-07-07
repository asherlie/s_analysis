#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

char** read_csv_fields(FILE* fp, int* n_fields){
    char* ln = NULL;
    size_t sz;
    int retsz = 10;
    char** ret = calloc(sizeof(char*), retsz);
    char* cursor, * startptr;
    ssize_t bytes_read;

    *n_fields = 0;

    if((bytes_read = getline(&ln, &sz, fp)) <= 0)return NULL;
    if(ln[bytes_read-1] == '\n')ln[--bytes_read] = ',';
    else{
        ln = realloc(ln, bytes_read+2);
        ln[bytes_read] = ',';
    }

    cursor = startptr = ln;
    while((cursor = strchr(cursor, ','))){
        *cursor = 0;
        if(*n_fields == retsz-1){
            retsz *= 2;
            ret = realloc(ret, retsz);
        }
        ret[(*n_fields)++] = strdup(startptr);
        cursor = startptr = cursor+1;
    }
    return ret;
}

char* getfields(FILE* fp){
    int nf;
    char* ret, * cursor;
    char** fields = read_csv_fields(fp, &nf);

    cursor = ret = calloc(nf, 20);
    for(int i = 0; i < nf; ++i){
        cursor += sprintf(cursor, "char* %s; ", fields[i]);
        /*strcpy();*/
        /*strcat*/
    }
    puts(ret);
    return ret;
}

/*
 * csvs are represented with separate lists - one for each column
 * each column can contain either char*, int, or float
*/
/*in htis case we need 7*3 lists only 7 will be used but we need to give an option of datatype*/

enum etype {s = 0, i, f, unknown};

/*should make a separate struct maybe and within it a list*/
struct csv_entry{
    union {
        char* str;
        int i;
        float f;
    }data;
};

struct csv_column{
    char* label;
    enum etype type;
    int n_entries, entry_cap;
    struct csv_entry* entries;
};

struct csv{
    int n_columns;
    struct csv_column* columns;
};

/*
 * values are considered integers if they can be converted to int without problem
 * floats are handled similarly, all leftover values are considered to be char*
 */
enum etype which_etype(char* raw, int* iret, float* fret){
    char* invalid;

    *iret = (int)strtol(raw, &invalid, 10);
    if(*invalid == 0){
        return i;
    }
    *fret = strtof(raw, &invalid);
    if(*invalid == 0){
        return f;
    }
    /*printf("%p %i\n", invalid, *invalid);*/
    /*if(invalid != 0*/
    /*if(strchr(raw, '.'))return f;*/
    return s;
}

// TODO: don't use which_etype() here
/*_Bool set_csv_entry(enum etype expected, struct csv_entry* e, char* raw){*/
/*_Bool set_csv_entry(struct csv* c, int column_idx, enum etype expected, char* raw){*/
// TODO: resize as needed
_Bool set_csv_entry(struct csv* c, int column_idx, char* raw){
    /* which_etype() used just for conversion here  */
    int iv;
    float fv;
    struct csv_entry* e;

    /* realloc all columns */
    if(c->columns[column_idx].n_entries == c->columns[column_idx].entry_cap){
        for(int i = 0; i < c->n_columns; ++i){
            c->columns[i].entry_cap *= 2;
            c->columns[i].entries = realloc(c->columns[i].entries, sizeof(struct csv_entry)*c->columns[i].entry_cap);
        }
        /*c->columns[column_idx].entries = realloc();*/
    }

    e = &c->columns[column_idx].entries[c->columns[column_idx].n_entries];

    if(which_etype(raw, &iv, &fv) != c->columns[column_idx].type)return 0;
    switch(c->columns[column_idx].type){
        case i:
            e->data.i = iv;
            break;
        case f:
            e->data.f = fv;
            break;
        case s:
        case unknown:
            e->data.str = raw;
    }
    ++c->columns[column_idx].n_entries;
    return 1;
}

void init_csv(struct csv* c, char** fields, int n_cols, int n_rows){
    c->n_columns = n_cols;
    c->columns = malloc(sizeof(struct csv_column)*c->n_columns);
    for(int i = 0; i < c->n_columns; ++i){
        c->columns[i].label = fields[i];
        /* type is marked as unknown until first row is processed */
        c->columns[i].type = unknown;
        c->columns[i].n_entries = 0;
        c->columns[i].entry_cap = n_rows;
        c->columns[i].entries = malloc(sizeof(struct csv_entry)*n_rows);
    }
}

void set_csv_types(struct csv* c, char** first_row){
    int iv;
    float fv;
    for(int i = 0; i < c->n_columns; ++i){
        c->columns[i].type = which_etype(first_row[i], &iv, &fv);
        /*set_csv_entry(c->columns[i].type, c->columns[i].entries, first_row[i]);*/
        set_csv_entry(c, i, first_row[i]);
    }
}

/* parse_csv() expects an open FILE* for a csv
 * this FILE* should be on the first datum of the csv file after the header
 */
void parse_csv(FILE* fp, struct csv* c){
    int nf;
    char** fields;

    fields = read_csv_fields(fp, &nf);
    set_csv_types(c, fields);
    while((fields = read_csv_fields(fp, &nf))){
        /*if(rownum == c->columns)*/
        /*printf("%i\n", nf);*/
        // last line is reported as having 6 fields
        for(int i = 0; i < c->n_columns; ++i){
            set_csv_entry(c, i, fields[i]);
        }
        /*puts(fields[0]);*/
    }
    /*while()*/
}

void load_csv(char* fn, struct csv* c){
    FILE* fp = fopen(fn, "r");
    int n_fields;
    char** fields = read_csv_fields(fp, &n_fields);
    init_csv(c, fields, n_fields, 1000);
    /* individual fields are being used as labels, no need to free */
    free(fields);
    parse_csv(fp, c);
    fclose(fp);
}

/*
 * this works.
 * i should use preprocessor defines to generate lookup functions that return different values
 * depending on the datatype and using this abstract function 
 * there should be separate functions to lookup by each field!!!
 * this is actually awesome!
 * preprocessor can generate all these
*/
struct csv_entry* csv_lookup(struct csv* c, char* field, int idx){
    for(int i = 0; i < c->n_columns; ++i){
        if(!strcmp(c->columns[i].label, field)){
            return c->columns[i].entries+idx;
            /*c->columns[i].data*/
        }
    }
    return NULL;
}

// all this really needs to do is return the correct type depending on type
// should be easy to write into a macro
// what's harder/impossible but would be awesome would be to defint a function to access each field programmatically
/*#define func(x, y) */

/*csvs should be represented as n separate lists, since all columns have the same number of elements*/

// generates a csv_entry struct
/*
 * #define csv_struct_builder(name, csv_fn, defstr) struct csv_##name{ \
 * defstr \
 * };
*/
/*file is taken in, a list of fields is read into an array of the perfect size*/
/*csv_struct_builder() is then called with sizeof(arr) and variable args*/


/*struct csv_entry*/
/*struct csv_data{*/
    
/*};*/

/*
 * this should be a generator that creates a struct with the relevant fields - date, open, wtvr
 * it raesd in first line of csv and creates a new struct!
*/

/*csv_struct_builder(another, "spy.csv", getfields("SPY.csv"))*/
/*struct csv_another xx = {.x = 33};*/

struct portfolio{
    int shares;
    float cost;
};

/* wrapping pct_change in deltainf in case more data is needed in the future */
struct deltainf{
    float pct_change;
};

enum trade_frequency {each_day = 0, each_n_days};
enum trade_trigger {red = 0, red_over_pct, indiscriminant, after_n_reds};

struct strategy{
    enum trade_frequency freq;
    enum trade_trigger trig;
    int n_red;
    float red_pct;
    int n_days;

    /*_Bool buy_each_red_day;*/
    /*int buy_after_n_red_days;*/
    /*_Bool buy_each_week_indiscriminantly;*/
    /*int8_t buy_on_dow;*/
    _Bool buy_nearest_dollar_amt;
    float dollars_per_purchase;
    int shares_per_purchase;
};

char* strat_to_txt(struct strategy* s){
    char* ret = calloc(100, 1);
    char* cursor = ret;
    cursor += sprintf(cursor, "buy ");

    if(s->buy_nearest_dollar_amt)
        cursor += sprintf(cursor, "all shares possible with $%f", s->dollars_per_purchase);
    else cursor += sprintf(cursor, "%i shares", s->shares_per_purchase);

    cursor += sprintf(cursor, " every ");
    if(s->freq == each_n_days)cursor += sprintf(cursor, "%i days", s->n_days);
    else cursor += sprintf(cursor, "day");

    if(s->trig == indiscriminant)cursor += sprintf(cursor, " indiscriminantly");
    else{
        cursor += sprintf(cursor, " when red");
        if(s->trig == red_over_pct)cursor += sprintf(cursor, " over %f%%", s->red_pct);
        if(s->trig == after_n_reds)cursor += sprintf(cursor, " for greater than %i days", s->n_days);
    }
    return ret;
}

/* computes the percentage inc/dec from row_a[field_a] -> row_b[field_b]
 * for ex. 1993-02-03['Close'] -> 1993-02-04['Open']
 */
struct deltainf process_delta(struct csv* c, int row_a, int row_b, char* field_a, char* field_b){
    struct deltainf ret = {0};
    struct csv_entry* ea, * eb;

    ea = csv_lookup(c, field_a, row_a);
    eb = csv_lookup(c, field_b, row_b);

    ret.pct_change = 100*((eb->data.f-ea->data.f)/((eb->data.f+ea->data.f)/2));
    return ret;
}

_Bool buy(struct portfolio* p, int shares, float price){
    p->shares += shares;
    p->cost += price*shares;

    return shares != 0;
}

void run_buy_strategy(struct csv* c, struct portfolio* p, const struct strategy* s){
    struct deltainf d;
    int red_streak = 0;
    int pc;
    int shares;
    float price;
    int days_since_purchase = s->n_days;

    for(int row = 1; row < c->columns->n_entries; ++row){
        shares = 0;
        price = csv_lookup(c, "Open", row)->data.f;
        d = process_delta(c, row-1, row, "Close", "Open");
        pc = d.pct_change;
        /*printf("%f\n", pc);*/
        if(pc < 0.)++red_streak;
        else red_streak = 0;
        /*printf("rs %i\n", red_streak);*/
        if(s->freq == each_day || (s->freq == each_n_days && s->n_days <= days_since_purchase)){
            if(s->trig == indiscriminant || (s->trig == red && pc < 0) || (s->trig == red_over_pct && pc < s->red_pct) ||
              (s->trig == after_n_reds && red_streak >= s->n_red)){
                // maybe bake this logic into buy()
                if(s->buy_nearest_dollar_amt)
                    shares = (int)s->dollars_per_purchase/price;
                else shares = s->shares_per_purchase;
            }
        }

        if(buy(p, shares, price))days_since_purchase = 0;
        else ++days_since_purchase;
        /*d = process_delta(c, row-1, row, "Close", "Close");*/
        /*printf("%s->%s: %.4f\n", csv_lookup(c, "Date", row-1)->data.str, csv_lookup(c, "Date", row)->data.str, d.pct_change);*/
    }
}

void p_portfolio(struct portfolio* p, float cur_price){
    printf("%i shares purchased at $%f per share\ntotal $%f spent\n$%f earned\n", 
        p->shares, p->cost/p->shares, p->cost, (cur_price*p->shares)-p->cost);
}

// TODO: generate reports and experiment with all different permutations of strategy
// TODO: strat_to_txt()
// TODO: write a list of strategies along with their dollars earned to a file
// so we can compare strategies
//
/*TODO: experiment with previous day's volume*/
int main(){
    /*printf("%i\n", (int)(100./3.));*/
    /*which_etype("23434");*/
    struct csv c;
    struct strategy s = {0};
    struct portfolio p;

    float cur_price;

    load_csv("SPY.csv", &c);
    printf("loaded csv with dimensions (%i, %i)\n", c.n_columns, c.columns->n_entries);

    s.freq = each_n_days;
    s.trig = indiscriminant;
    s.n_days = 20;

    s.buy_nearest_dollar_amt = 0;
    /*s.dollars_per_purchase = 1000.;*/
    s.shares_per_purchase = 2;

    run_buy_strategy(&c, &p, &s);

    cur_price = csv_lookup(&c, "Open", c.columns->n_entries-1)->data.f;
    p_portfolio(&p, cur_price);
    puts(strat_to_txt(&s));
    /*printf("dollars earned: %f\n", p.shares*cur_price);*/
    return EXIT_SUCCESS;
}
