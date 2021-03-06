#!/usr/bin/env awk -f

BEGIN {
  sym_prefix = "_"
  split("\
        _jet_aligned_alloc \
        _jet_calloc \
        _jet_dallocx \
        _jet_free \
        _jet_free_sized \
        _jet_free_aligned_sized \
        _jet_mallctl \
        _jet_mallctlbymib \
        _jet_mallctlnametomib \
        _jet_malloc \
        _jet_malloc_conf \
        _jet_malloc_conf_2_conf_harder \
        _jet_malloc_message \
        _jet_malloc_stats_print \
        _jet_malloc_usable_size \
        _jet_mallocx \
        _jet_smallocx_ed2ff69fc6e6e6150631ae07bbbf57eb96de5d0c \
        _jet_nallocx \
        _jet_posix_memalign \
        _jet_rallocx \
        _jet_realloc \
        _jet_sallocx \
        _jet_sdallocx \
        _jet_xallocx \
        _jet_valloc \
        _jet_malloc_size \
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
