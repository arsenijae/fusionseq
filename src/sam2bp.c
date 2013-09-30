#include <bios/log.h>
#include <bios/format.h>
#include <bios/linestream.h>
#include <mrf/sam.h>

typedef struct {
  char *target;
  char *read;
  int position;
} BreakPoint;

typedef struct {
  char *tileCoordinate1;
  char *tileCoordinate2;
  Array breakPoints; // of type BreakPoint*
} SuperBreakPoint;

static int sortBreakPointsByTargetAndOffset(BreakPoint *a, BreakPoint *b)  {
  int diff = strcmp (a->target,b->target);
  if (diff != 0) {
    return diff;
  }
  return b->position - a->position;
}

static int sortSuperBreakPointsBySupport(SuperBreakPoint *a, 
                                         SuperBreakPoint *b) {
  return arrayMax (b->breakPoints) - arrayMax (a->breakPoints);
}

int main (int argc, char *argv[]) {
  SamParser* parser = samparser_from_file("-");
  Array sam_entries = samparser_get_all_entries(parser);
  samparser_free(parser); 
  Array breakpoints = arrayCreate(10000, BreakPoint);
  for (int i = 0; i < arrayMax(sam_entries); ++i) {
    SamEntry* sam_entry = arrp(sam_entries, 0, SamEntry);
    BreakPoint* bp = arrayp(breakpoints, arrayMax(breakpoints), BreakPoint);
    bp->target = hlr_strdup(sam_entry->rname);
    bp->read = hlr_strdup(sam_entry->seq);
    bp->position = sam_entry->pos;
  }

  arraySort(breakpoints, (ARRAYORDERF) sortBreakPointsByTargetAndOffset);
  Array super_breakpoints = arrayCreate(1000, SuperBreakPoint);
  int i = 0;
  while (i < arrayMax(breakpoints)) {
    char* target_copy = NULL;
    BreakPoint* bp = arrp(breakpoints, i, BreakPoint);
    SuperBreakPoint* super_bp = 
        arrayp(super_breakpoints, arrayMax(super_breakpoints), SuperBreakPoint);
    super_bp->breakPoints = arrayCreate(100, BreakPoint*);
    strReplace(&target_copy, bp->target);
    char* pos = strchr(target_copy, '|');
    if (pos == NULL) {
      die("Unexpected target: %s", target_copy);
    }
    *pos = '\0';
    super_bp->tileCoordinate1 = hlr_strdup(target_copy);
    super_bp->tileCoordinate2 = hlr_strdup(pos + 1);
    arrayPush(super_bp->breakPoints, bp, BreakPoint*);
    int j = i + 1;
    while (j < arrayMax(breakpoints)) {
      BreakPoint* next_bp = arrp(breakpoints, j, BreakPoint);
      if (strEqual(bp->target, next_bp->target)) {
        arrayPush(super_bp->breakPoints, next_bp, BreakPoint*);
      } else {
        break;
      }
      j++;
    }
    i = j;
  }
  arraySort(super_breakpoints, (ARRAYORDERF) sortSuperBreakPointsBySupport);
  for (int i = 0; i < arrayMax(super_breakpoints); i++) {
    SuperBreakPoint* super_bp = arrp(super_breakpoints,i,SuperBreakPoint);
    printf ("%s,%s,", super_bp->tileCoordinate1, super_bp->tileCoordinate2);
    for (int j = 0; j < arrayMax(super_bp->breakPoints); j++) {
      BreakPoint* bp = arru(super_bp->breakPoints, j, BreakPoint*);
      printf("%d:%s%s", bp->position, bp->read, 
             j < arrayMax(super_bp->breakPoints) - 1 ? "|" : "\n");
    }
  }
  
  return EXIT_SUCCESS;
}
