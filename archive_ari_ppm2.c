#include <stdio.h>
#include <stdlib.h>

enum { emnt_chrs=256, EOF_sym=257, emnt_bit_code=16, emnt_sym=257,
       max_val=65535L, first_qtr=16384L, half=32768L, third_qtr=49152L,
       max_freq=16383};

struct tables {
  int sym_from_chr[emnt_chrs],
      chr_from_sym[emnt_sym+1],
      freq_s[emnt_sym+1],
      seg_freq[emnt_sym+1];
};

struct other_values {
  long low, high, value;
  int rest_bits, bits_to_follow, buf;
};

void set_other_values(struct other_values *new)
{
  new->low=0;
  new->high=max_val;
  new->value=0;
  new->bits_to_follow=0;
  new->buf=0;
}

int cmpstrings(char *firststr, char *secondstr)
{
  int i=0;
  while ((firststr[i]==secondstr[i]) && (firststr[i]!=0)) {
    i++;
  }
  return (firststr[i]==secondstr[i]);
}

void write_bit(int bit, struct other_values *all_values, FILE *name)
{
  (all_values->buf)>>=1;
  if (bit) (all_values->buf)|=0x80;
  (all_values->rest_bits)--;
  if ((all_values->rest_bits)==0) {
    fputc(all_values->buf,name);
    all_values->rest_bits=8;
  }
}

int read_bit(struct other_values *all_values, FILE *name)
{
  int tmp;

  if ((all_values->rest_bits)==0) {
    (all_values->buf)=fgetc(name);
    (all_values->rest_bits)=8;
  }
  tmp=(all_values->buf)&1;
  (all_values->buf)>>=1;
  (all_values->rest_bits)--;
  return tmp;
}

void set_model(struct tables *table)
{
  int i;
  for (i=0;i<=emnt_sym;i++) {
    table->freq_s[i]=1;
    table->seg_freq[i]=emnt_sym-i;
  }
  table->freq_s[0]=0;
  for (i=0;i<emnt_chrs;i++) {
    table->sym_from_chr[i]=i+1;
    table->chr_from_sym[i+1]=i;
  }
}

void update_model(int sym, struct tables *table)
{
  int i, new_chr, cur_chr, help=0;
  if ((table->seg_freq)[0]==max_freq) {
    for (i=emnt_sym;i>=0;i--) {
      (table->freq_s)[i]=((table->freq_s)[i]+1)/2;
      (table->seg_freq)[i]=help;
      help+=(table->freq_s)[i];
    }
  }
  for (i=sym;(table->freq_s)[i]==(table->freq_s)[i-1];i--);
  if (i<sym) {
    new_chr=(table->chr_from_sym)[i];
    cur_chr=(table->chr_from_sym)[sym];
    (table->chr_from_sym)[i]=cur_chr;
    (table->chr_from_sym)[sym]=new_chr;
    (table->sym_from_chr)[new_chr]=sym;
    (table->sym_from_chr)[cur_chr]=i;
  }
  ((table->freq_s)[i])++;
  while (i>0) {
    i--;
    ((table->seg_freq)[i])++;
  }
}

void bit_plus_follow(int bit, struct other_values *all_values, FILE *f_second)
{
  write_bit(bit, all_values, f_second);
  while ((all_values->bits_to_follow)>0) {
    write_bit(!bit, all_values, f_second);
    (all_values->bits_to_follow)--;
  }
}

void encode_sym
(int sym, struct other_values *all_values, struct tables *table, FILE *f_second)
{
  long range;

  range=(long)((all_values->high)-(all_values->low))+1;
  (all_values->high)=
    (all_values->low)+(range*table->seg_freq[sym-1])/table->seg_freq[0]-1;
  (all_values->low)=
    (all_values->low)+(range*table->seg_freq[sym])/table->seg_freq[0];
  for (;;) {
    if ((all_values->high)<half) {
      bit_plus_follow(0, all_values, f_second);
    } else 
    if ((all_values->low)>=half) {
      bit_plus_follow(1, all_values, f_second);
      (all_values->low)-=half;
      (all_values->high)-=half;
    } else 
    if (((all_values->low)>=first_qtr) && ((all_values->high)<third_qtr)) {
      (all_values->bits_to_follow)++;
      (all_values->low)-=first_qtr;
      (all_values->high)-=first_qtr;
    } else {
      break;
    } 
    (all_values->low)=2*(all_values->low);
    (all_values->high)=2*(all_values->high)+1;
  }
}

void change_seg_d(struct other_values *all_values, FILE *f_first)
{
  for (;;) {
    if ((all_values->high)<half) {
    }
    else {
      if ((all_values->low)>=half) {
        (all_values->value)-=half;
        (all_values->low)-=half;
        (all_values->high)-=half;
      }
      else {
        if (((all_values->low)>=first_qtr) && ((all_values->high)<third_qtr)) {
          (all_values->value)-=first_qtr;
          (all_values->low)-=first_qtr;
          (all_values->high)-=first_qtr;
        }
        else {
          break;
        }
      }
    }
    (all_values->low)=(all_values->low)*2;
    (all_values->high)=(all_values->high)*2+1;
    (all_values->value)=(all_values->value)*2+read_bit(all_values, f_first);
  }
}

int decode_sym
(struct other_values *all_values, struct tables *table, FILE *f_first)
{
  long range, seg;
  int  sym;

  range=((all_values->high)-(all_values->low))+1;
  seg=
    ((((all_values->value)-(all_values->low))+1)*(table->seg_freq)[0]-1)/range;
  for (sym=1; (table->seg_freq)[sym]>seg;sym++);
  (all_values->high)=
    (all_values->low)+(range*(table->seg_freq)[sym-1])/(table->seg_freq)[0]-1;
  (all_values->low)=
    (all_values->low)+(range*(table->seg_freq)[sym])/(table->seg_freq)[0];
  change_seg_d(all_values, f_first);
  return sym;
}



void done_encoding(struct other_values *all_values, FILE *f_second)
{
  (all_values->bits_to_follow)++;
  if ((all_values->low)<first_qtr) {
    bit_plus_follow(0, all_values, f_second);
  }
  else {
    bit_plus_follow(1, all_values, f_second);
  }
}

int check_ari_ppm(int argc, char **argv)
{
  if (argc==4) {
    return 1;
  } else
  if (cmpstrings("ari", argv[4])) {
    return 1;
  } else return 0;
}

void encode_all_sym_ari(struct other_values *all_values,
                        struct tables *table, FILE *f_first, FILE *f_second)
{
  int sym, chr;
  all_values->rest_bits=8;
  for (;;) {
    chr=fgetc(f_first);
    if (chr==EOF) break;
    sym=table->sym_from_chr[chr];
    encode_sym(sym, all_values, table, f_second);
    update_model(sym, table);
  }
  encode_sym(EOF_sym, all_values, table, f_second);
  done_encoding(all_values, f_second);
  fputc(all_values->buf>>all_values->rest_bits, f_second);
}


void decode_all_sym_ari(struct other_values *all_values, struct tables *table, 
                        FILE *f_first, FILE *f_second)
{
  int i, sym, chr;
  all_values->rest_bits=0;
  all_values->value=0;
  for (i=1;i<=emnt_bit_code;i++) {
    (all_values->value)=2*(all_values->value)+read_bit(all_values, f_first);
  }
  for (;;) {
    sym=decode_sym(all_values, table, f_first);
    if (sym==EOF_sym) break;
    chr=table->chr_from_sym[sym];
    fputc(chr, f_second);
    update_model(sym, table);
  }
}

void ppm(struct other_values *all_values, int argc, char **argv,
         FILE *f_first, FILE *f_second)
{
  struct tables ppm[emnt_chrs][emnt_chrs];
  int i, j,  prev2_chr='a', prev_chr='a', chr, sym;
  for (j=0;j<emnt_chrs;j++) {
    for (i=0;i<emnt_chrs;i++) {
      set_model(&(ppm[j][i]));
    }
  }
  if (cmpstrings(argv[1],"c")) {
    all_values->rest_bits=8;
    for (;;) {
      chr=fgetc(f_first);
      if (chr==EOF) break;
      sym=ppm[prev_chr][prev2_chr].sym_from_chr[chr];
      encode_sym(sym, all_values, &ppm[prev_chr][prev2_chr], f_second);
      update_model(sym, &ppm[prev_chr][prev2_chr]);
      prev2_chr=prev_chr;
      prev_chr=chr;
    }
    encode_sym(EOF_sym, all_values, &ppm[prev_chr][prev2_chr], f_second);
    done_encoding(all_values, f_second);
    fputc(all_values->buf>>all_values->rest_bits, f_second);
    }
  if (cmpstrings(argv[1],"d")) {
    all_values->rest_bits=0;
    all_values->value=0;
    for (i=1;i<=emnt_bit_code;i++) {
      (all_values->value)=2*(all_values->value)+read_bit(all_values, f_first);
    }
    for (;;) {
      sym=decode_sym(all_values, &ppm[prev_chr][prev2_chr], f_first);
      if (sym==EOF_sym) break;
      chr=ppm[prev_chr][prev2_chr].chr_from_sym[sym];
      fputc(chr, f_second);
      update_model(sym, &ppm[prev_chr][prev2_chr]);
      prev2_chr=prev_chr;
      prev_chr=chr;
    }
  }
}

int main(int argc, char **argv)
{
  FILE *f_first, *f_second;
  struct other_values all_values;

  set_other_values(&all_values);
  f_first=fopen(argv[2],"r");
  f_second=fopen(argv[3],"w");

  if (check_ari_ppm(argc, argv)) {
    struct tables table;
    set_model(&table);
    if (cmpstrings(argv[1],"c")) {
      encode_all_sym_ari(&all_values, &table, f_first, f_second);
    }
    if (cmpstrings(argv[1],"d")) {
      decode_all_sym_ari(&all_values, &table, f_first, f_second);
    }
  }
  else {
    ppm(&all_values, argc, argv, f_first, f_second);
  }
  fclose(f_first);
  fclose(f_second);
  return 0;
}

