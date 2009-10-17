#include <stdio.h>
#include <unistd.h>
#include "fileXio_rpc.h"

#include "SMS_CDVD.h"
#include "SMS_CDDA.h"
#include "graphic.h"
#include "getrte.h"
#include "configuration.h"
#include "modules.h"

static char copybuffer[1024];
char rtePath[1024] = "cdfs:modules/mod203";
const char *moduleList[] = {
	"iopintr.irx",
	"dmarelay.irx",
	"libsd.irx",
	"mcman.irx",
	"mcserv.irx",
	"padman.irx",
	"sdrdrv.irx",
	"sio2man.irx",
	NULL
};

static int real_copyRTEModules(void *arg) {
	int i;

	for (i = 0; moduleList[i] != NULL; i++) {
		const char *filename = moduleList[i];
		static char inPath[1024];
		static char outPath[1024];
		FILE *fin;
		FILE *fout;
		int size;
		int filesize;
		int copiedSize;

		graphic_setPercentage(0, filename);

		snprintf(inPath, sizeof(inPath), "%s/%s", rtePath, filename);
		snprintf(outPath, sizeof(outPath), CONFIG_DIR "/%s", filename);

		fin = fopen(inPath, "rb");
		if (fin != NULL) {
			fseek(fin, 0, SEEK_END);
			filesize = ftell(fin);
			fseek(fin, 0, SEEK_SET);
			copiedSize = 0;

			fout = fopen(outPath, "wb");
			if (fout == NULL) {
				fileXioMkdir(CONFIG_DIR, 0777);
				fout = fopen(outPath, "wb");
			}
			if (fout != NULL) {
				while(!feof(fin)) {
					size = fread(copybuffer, 1, sizeof(copybuffer), fin);
					if (size > 0) {
						if (fwrite(copybuffer, 1, size, fout) != size) {
							fclose(fout);
							fclose(fin);
							unlink(outPath);
							error_printf("Failed to write file %s (MC0 full?).", outPath);
							break;
						}
						copiedSize += size;
						graphic_setPercentage((100 * copiedSize) / filesize, filename);
					} else {
						break;
					}
				}
				fclose(fout);
				graphic_setPercentage(100, filename);
			} else {
				fclose(fin);
				error_printf("Failed to open file %s for writing.", outPath);
				break;
			}
			fclose(fin);
		} else {
			error_printf("Failed to open file %s for reading.", inPath);
			break;
		}
	}
	return 0;
}

int copyRTEModules(void *arg) {
	DiskType type;
	int rv;

	setEnableDisc(-1);

	if (isDVDVSupported()) {
		type = CDDA_DiskType();
	
		/* Detect disk type, so loading will work. */
		if (type == DiskType_DVDV) {
			CDVD_SetDVDV(1);
		} else {
			CDVD_SetDVDV(0);
		}
	}

	rv = real_copyRTEModules(arg);

	if (isDVDVSupported()) {
		/* Always stop CD/DVD when an error happened. */
		CDVD_Stop();
		CDVD_FlushCache();
	}

	setEnableDisc(0);

	if (getErrorMessage() == NULL) {
		graphic_setPercentage(0, NULL);
	}

	return rv;
}
