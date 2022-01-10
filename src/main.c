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

/* 执行cfg文件 ---------------------------------------------------------------*/
static int proccfgfile(char cfgfile[])
{
	/* 局部变量定义 ======================================================= */
	FILE* fp = NULL;					// cfg文件指针
	char* p;							// cfg文件每行字符指针
	char tmp[MAXSTRPATH] = { '\0' };	// cfg文件每行字符串
	/* ==================================================================== */

	// initialization
	PPP_Glo.prcType      = -1;
	PPP_Glo.outFolder[0] = '\0';
	PPP_Glo.inputPath[0] = '\0';

	// 尝试打开cfg文件，若不存在则返回0
	if ((fp = fopen(cfgfile, "r")) == NULL) {
		printf("*** ERROR: open configure file failed, please check it!\n");
		return 0;
	}

	/* 获取cfg文件中obs文件的处理模式&路径 ----------------------------------
	 *  1.PPP_Glo.prcType		| 处理模式 [0] 单文件 [1] 批处理
		2.PPP_Glo.inputPath		| 文件路径
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

				trimSpace(PPP_Glo.inputPath);					// 清除路径空格
				cutFilePathSep(PPP_Glo.inputPath);				// 截取最后一级文件/目录名
			}
			else {
				printf("*** ERROR: read obs files path error!");
				return 0;
			}
			break;
		}
	}
	fclose(fp);

	// 根据prcType选择后续处理步骤，[0] 单文件 [1] 批处理 [else] 异常
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

	t1 = clock(); //获取程序开始时间

	if (argc == 1) {
		printf("\n * The input command-line parameter indicating configure file is lost, please check it!\n");
		return 0;
	}
	else {
		cfgfile = argv[1];
	}
	/* find processing configure file */
	proccfgfile(cfgfile);

	t2 = clock(); //获取程序结束时间

	printf("\n * The total time for running the program: %6.3f seconds\n%c", (double)(t2 - t1) / CLOCKS_PER_SEC, '\0');
	//printf("Press any key to exit!\n");
	//getchar();
	system("pause");


	return 0;
}