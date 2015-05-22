/* A simple frontend for libxmp */
/* This file is in public domain */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <xmp.h>

static void display_data(struct xmp_module_info *mi, struct xmp_frame_info *fi)
{
	printf("%3d/%3d %3d/%3d\r",
	       fi->pos, mi->mod->len, fi->row, fi->num_rows);

	fflush(stdout);
}

static void update_cksum(uint8_t *ck, uint8_t *buf, size_t len)
{
	size_t i;
	uint8_t tmp = 0;

	for (i = 0; i < len; i++)
		tmp ^= buf[i];

	*ck ^= tmp;
}

int main(int argc, char **argv)
{
	xmp_context ctx;
	struct xmp_module_info mi;
	struct xmp_frame_info fi;
	int row, i;

	uint8_t ck = 0xff;

	ctx = xmp_create_context();

	for (i = 1; i < argc; i++) {
		if (xmp_load_module(ctx, argv[i]) < 0) {
			fprintf(stderr, "%s: error loading %s\n", argv[0],
				argv[i]);
			continue;
		}

		if (xmp_start_player(ctx, 44100, 0) == 0) {

			/* Show module data */

			xmp_get_module_info(ctx, &mi);
			printf("%s (%s)\n", mi.mod->name, mi.mod->type);

			/* Play module */

			row = -1;
			int i = 10;
			while (xmp_play_frame(ctx) == 0 && i --> 0) {
				xmp_get_frame_info(ctx, &fi);
				if (fi.loop_count > 0)
					break;

				update_cksum(&ck, fi.buffer, fi.buffer_size);

				if (fi.row != row) {
					display_data(&mi, &fi);
					row = fi.row;
				}
			}
			xmp_end_player(ctx);
		}

		xmp_release_module(ctx);
		printf("\n");
	}

	xmp_free_context(ctx);

	printf("cksum = 0x%02x\n", ck);

	return 0;
}
