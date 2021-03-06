#!/usr/bin/env awk -f

BEGIN {
  sym_prefix = "_"
  split("\
        _je_aligned_alloc \
        _je_calloc \
        _je_dallocx \
        _je_free \
        _je_free_sized \
        _je_free_aligned_sized \
        _je_mallctl \
        _je_mallctlbymib \
        _je_mallctlnametomib \
        _je_malloc \
        _je_malloc_conf \
        _je_malloc_conf_2_conf_harder \
        _je_malloc_message \
        _je_malloc_stats_print \
        _je_malloc_usable_size \
        _je_mallocx \
        _je_smallocx_ed2ff69fc6e6e6150631ae07bbbf57eb96de5d0c \
        _je_nallocx \
        _je_posix_memalign \
        _je_rallocx \
        _je_realloc \
        _je_sallocx \
        _je_sdallocx \
        _je_xallocx \
        _je_valloc \
        _je_malloc_size \
        _pthread_create \
        ", exported_symbol_names)
  # Store exported symbol names as keys in exported_symbols.
  for (i in exported_symbol_names) {
    exported_symbols[exported_symbol_names[i]] = 1
  }
}

# Process 'nm -P -t d <c_source.o>' output.  GNU nm omits a trailing size
# field when the ELF symbol size is zero, so accept both three and four fields.
#
# Handle lines like:
#   opt_junk D 8 1
#   tsd_tls L 0 1920
#   prof_active_get T 0
((NF == 3 || NF == 4) &&
 $2 ~ /^[ABCDGLNRSTVW]$/ &&
 !($1 in exported_symbols) &&
 $1 ~ /^[A-Za-z_][A-Za-z0-9_]*$/ &&
 $3 ~ /^[0-9]+$/) {
  print substr($1, 1+length(sym_prefix), length($1)-length(sym_prefix))
  next
}

# Process 'dumpbin /SYMBOLS <c_source.obj>' output.
#
# Handle lines like:
#   353 00008098 SECT4  notype       External     | opt_junk
#   3F1 00000000 SECT7  notype ()    External     | malloc_initialized
($3 ~ /^SECT[0-9]+/ && $(NF-2) == "External" && !($NF in exported_symbols)) {
  print $NF
}
