/* GDC -- D front-end for GCC
   Copyright (C) 2004 David Friedman
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
 
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
 
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

typedef struct {
    unsigned size;
    unsigned offset;
    char * field_name;
    char * type_name;
    int check;
} FieldInfo;

char * int_type_name(unsigned size, int is_signed)
{
    switch (size) {
    case 1: return is_signed ? "byte" : "ubyte";
    case 2: return is_signed ? "short" : "ushort";
    case 4: return is_signed ? "int" : "uint";
    case 8: return is_signed ? "long" : "ulong";
    default:
	assert(0);
	return NULL;
    }     
}

#define INT_FIELD(fi, field) \
fi.size = sizeof(rec.field); \
fi.offset = (char*)&rec.field - (char*)&rec; \
fi.field_name = #field; \
fi.check = 1; \
{ \
    typeof(rec.field) zerv; \
    typeof(rec.field) negv; \
    memset(& zerv, 0x00, sizeof(rec.field)); \
    memset(& negv, 0xff, sizeof(rec.field)); \
    fi.type_name = int_type_name(fi.size, negv < zerv); \
}

#define ADD_FIELD(fi, type, field) \
fi.size = sizeof(rec.field); \
fi.offset = (char*)&rec.field - (char*)&rec; \
fi.field_name = #field; \
fi.type_name = type; \
fi.check = 0;

#define OPAQUE_TYPE(type) \
printf("struct %s { ubyte[%u] opaque; }\n", #type, sizeof(type));

#define INT_TYPE(type) \
{ \
    type zerv; \
    type negv; \
    memset(& zerv, 0x00, sizeof(type)); \
    memset(& negv, 0xff, sizeof(type)); \
    printf("alias %s %s;\n", int_type_name(sizeof(type), negv < zerv), \
	#type); \
}

//int fi_compare(const FieldInfo * pa, const FieldInfo * pb)
int fi_compare(const void * pa, const void * pb)
{
    return (int)((FieldInfo*)pa)->offset - (int)((FieldInfo*)pb)->offset;
}

void finish_struct_ex(FieldInfo * fields, unsigned field_count, unsigned size, char * name, char * extra)
{
    unsigned pad_idx = 1;
    unsigned last_offset = (unsigned) -1;
    unsigned next_offset = 0;
    FieldInfo *p_fi = fields;
    FieldInfo *p_fi_end = p_fi + field_count;
    int in_union = 0;
    
    qsort(p_fi, field_count, sizeof(FieldInfo), & fi_compare);
    printf("struct %s {\n", name);

    for (p_fi = fields; p_fi != p_fi_end; p_fi++) {
	if (p_fi->offset == last_offset) {
	    // in a union
	} else if (in_union) {
	    printf("    }\n");
	    in_union = 0;
	}

	if (p_fi->offset > next_offset) {
	    printf("   ubyte[%u] ___pad%u;\n", p_fi->offset - next_offset, pad_idx++);
	}
	
	if (! in_union &&
	    p_fi + 1 != p_fi_end &&
	    p_fi->offset == p_fi[1].offset) {
	    in_union = 1;
	    printf("    union {\n");
	}

	printf("%s    %s %s;\n", in_union?"    ":"", p_fi->type_name, p_fi->field_name);
	last_offset = p_fi->offset;
	next_offset = last_offset + p_fi->size; /* doesn't handle alignment */
    }
    if (next_offset < size)
	printf("   ubyte[%u] ___pad%u;\n", size - next_offset, pad_idx++);

    if (extra)
	printf("%s\n", extra);
    
    printf("}\n\n");

    for (p_fi = fields; p_fi != p_fi_end; p_fi++) {
	// possible bug in DMD prevents use of '.offset'?
	if (p_fi->check)
	    printf("static assert(%s.%s.offsetof == %u);\n",
		name, p_fi->field_name, p_fi->offset);
    }
    printf("static assert(%s.sizeof == %u);\n", name, size);
    printf("\n\n");

}
void finish_struct(FieldInfo * fields, unsigned field_count, unsigned size, char * name)
{
    finish_struct_ex(fields, field_count, size, name, NULL);
}

#define CES(sym) printf("  %s = %d,\n", #sym, (sym));
#define CESA(sym, val) printf("  %s = %d,\n", #sym, (val));
#define CESX(sym) printf("  %s = 0x%x,\n", #sym, (sym));
#define CESZ(sym) printf("  %s = 0,\n", #sym);
