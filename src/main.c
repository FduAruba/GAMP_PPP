#include <stdarg.h>
#include "gamp.h"

#define MAXFILE     16                  /* max number of input files */

/* show message --------------------------------------------------------------*/
extern int showmsg(char* format, ...)
{
	va_list arg;
	va_start(arg, format); vfprintf(stderr, format, arg); va_end(arg);
	fprintf(stderr, "\r");
	return 0;
}
extern void settspan(gtime_t ts, gtime_t te) {}
extern void settime(gtime_t time) {}

/* ִ��cfg�ļ� ---------------------------------------------------------------*/
static int proccfgfile(char cfgfile[])
{
	/* �ֲ��������� ======================================================= */
	FILE* fp = NULL;					// cfg�ļ�ָ��
	char* p;							// cfg�ļ�ÿ���ַ�ָ��
	char tmp[MAXSTRPATH] = { '\0' };	// cfg�ļ�ÿ���ַ���
	/* ==================================================================== */

	// initialization
	PPP_Glo.prcType      = -1;
	PPP_Glo.outFolder[0] = '\0';
	PPP_Glo.inputPath[0] = '\0';

	// ���Դ�cfg�ļ������������򷵻�0
	if ((fp = fopen(cfgfile, "r")) == NULL) {
		printf("*** ERROR: open configure file failed, please check it!\n");
		return 0;
	}

	/* ��ȡcfg�ļ���obs�ļ��Ĵ���ģʽ&·�� ----------------------------------
	 *  1.PPP_Glo.prcType		| ����ģʽ [0] ���ļ� [1] ������
		2.PPP_Glo.inputPath		| �ļ�·��
	 ---------------------------------------------------------------------- */
	while (!feof(fp)) {
		tmp[0] = '\0';
		fgets(tmp, MAXSTRPATH, fp);
		if ((tmp != NULL) && (tmp[0] == '#')) continue;

		if (strstr(tmp, "obs file/folder")) {
			p = strrchr(tmp, '=');
			sscanf(p + 1, "%d", &PPP_Glo.prcType);

			tmp[0] = '\0';
			if (fgets(tmp, MAXSTRPATH, fp)) {
				p = strrchr(tmp, '=');
				sscanf(p + 1, "%[^,]", PPP_Glo.inputPath);

				trimSpace(PPP_Glo.inputPath);					// ���·���ո�
				cutFilePathSep(PPP_Glo.inputPath);				// ��ȡ���һ���ļ�/Ŀ¼��
			}
			else {
				printf("*** ERROR: read obs files path error!");
				return 0;
			}
			break;
		}
	}
	fclose(fp);

	// ����prcTypeѡ����������裬[0] ���ļ� [1] ������ [else] �쳣
	switch (PPP_Glo.prcType) {
	case 0:
		procOneFile(PPP_Glo.inputPath, cfgfile, 0, 1);
	case 1:
		batchProc(PPP_Glo.inputPath, cfgfile);
	default:
		printf("*** ERROR: read obs files path error!");
		return 0;
	}

	return 1;
}

/* GAMP main -------------------------------------------------------------*/
int main(int argc, char** argv)
{
	//char cfgfile[1000]="C:\\mannual_GAMP\\Examples\\2017244\\gamp.cfg";
	char* cfgfile;
	long t1, t2;

	t1 = clock(); //��ȡ����ʼʱ��

	if (argc == 1) {
		printf("\n * The input command-line parameter indicating configure file is lost, please check it!\n");
		return 0;
	}
	else {
		cfgfile = argv[1];
	}
	/* find processing configure file */
	proccfgfile(cfgfile);

	t2 = clock(); //��ȡ�������ʱ��

	printf("\n * The total time for running the program: %6.3f seconds\n%c", (double)(t2 - t1) / CLOCKS_PER_SEC, '\0');
	//printf("Press any key to exit!\n");
	//getchar();
	system("pause");


	return 0;
}