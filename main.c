/** @file

Copyright (c) 2025, Cyprien Laplace. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause

**/

#include <Base.h>
#include <IndustryStandard/Acpi.h>

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

void *pptt;
unsigned package;
unsigned core;

void do_cache(unsigned off)
{
	EFI_ACPI_6_4_PPTT_STRUCTURE_CACHE *c = pptt + off;
	char *size = "";

	if (c->Flags.SizePropertyValid) {
		if (c->Size > 1024*1024)
			asprintf(&size, "\\n%u MB", c->Size/1024/1024);
		else
			asprintf(&size, "\\n%u KB", c->Size/1024);
	}

	if (c->Flags.CacheTypeValid && c->Attributes.CacheType == 0) {
		printf("c%x [label=\"D$%s\",shape=diamond];\n", off, size);
	} else if (c->Flags.CacheTypeValid && c->Attributes.CacheType == 1) {
		printf("c%x [label=\"I$%s\",shape=diamond];\n", off, size);
	} else {
		printf("c%x [label=\"$%s\",shape=diamond];\n", off, size);
	}

	if (c->NextLevelOfCache != 0) {
		printf("c%x -> c%x;\n", off, c->NextLevelOfCache);
	}
}

void do_processor(unsigned off)
{
	EFI_ACPI_6_4_PPTT_STRUCTURE_PROCESSOR *p = pptt + off;
	UINT32 *priv = pptt + off + sizeof *p;
	unsigned res;
	const char *leaf = p->Flags.NodeIsALeaf ? ",style=filled":"";

	//fprintf(stderr, "flags: %x\n", p->Flags);
	if (p->Flags.PhysicalPackage) {
		printf("n%x [label=\"package %u\",shape=box];\n", off, package++);
	} else if (p->Flags.AcpiProcessorIdValid) {
		printf("n%x [label=\"core\\n%x\",shape=circle%s];\n", off, p->AcpiProcessorId, leaf);
	} else {
		printf("n%x [shape=point];\n", off);
	}
	if (p->Parent != 0) {
		printf("n%x -> n%x;\n", p->Parent, off);
	}

	for (res = 0; res < p->NumberOfPrivateResources; ++res) {
		printf("n%x -> c%x;\n", off, priv[res]);
	}
}

int main()
{
	int fd = open("PPTT.aml", O_RDONLY);
	off_t len = lseek(fd, 0, SEEK_END);
	unsigned off = sizeof(EFI_ACPI_6_4_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_HEADER);

	pptt = mmap(NULL, len, PROT_READ, MAP_SHARED, fd, 0);

	printf("digraph graphname {\n");
	//printf("center=true\n");
	printf("overlap_scaling=0\n");
	//fprintf(stderr, "fd %d len %ld pptt %p\n", fd, len, pptt);
	while (off < len) {
		EFI_ACPI_6_4_PPTT_STRUCTURE_HEADER *h = pptt + off;
		//fprintf(stderr, "%04x: type %u\n", off, h->Type);
		switch (h->Type) {
		case EFI_ACPI_6_4_PPTT_TYPE_PROCESSOR:
			do_processor(off);
			break;
		case EFI_ACPI_6_4_PPTT_TYPE_CACHE:
			do_cache(off);
			break;
		}
		off += h->Length;
	}
	printf("}\n");

	return 0;
}
