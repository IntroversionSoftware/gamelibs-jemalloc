#!/usr/bin/env awk -f

BEGIN {
  sym_prefix = ""
  split("\
        je_aligned_alloc \
        je_calloc \
        je_dallocx \
        je_free \
        je_free_sized \
        je_free_aligned_sized \
        je_mallctl \
        je_mallctlbymib \
        je_mallctlnametomib \
        je_malloc \
        je_malloc_conf \
        je_malloc_conf_2_conf_harder \
        je_malloc_message \
        je_malloc_stats_print \
        je_malloc_usable_size \
        je_mallocx \
        je_smallocx_ed2ff69fc6e6e6150631ae07bbbf57eb96de5d0c \
        je_nallocx \
        je_posix_memalign \
        je_rallocx \
        je_realloc \
        je_sallocx \
        je_sdallocx \
        je_xallocx \
        tls_callback \
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
