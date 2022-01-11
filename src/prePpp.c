#include <sys/types.h>
#include <dirent.h>
#include "gamp.h"

//init PPP_Global struct
static void initGlobal(PPPGlobal_t* ppp)
{
	int i;
	gtime_t gt0 = { 0 };
	ssatEx_t ssat_EX0 = { {0} };

	ppp->prcOpt_Ex.navSys  = SYS_GPS;
	ppp->prcOpt_Ex.solType = 0;
	ppp->prcOpt_Ex.posMode = PMODE_PPP_STATIC;
	ppp->prcOpt_Ex.rcvType[0] = '\0';

	ppp->sitName[0] = '\0';
	for (i = 0; i < 4; i++) ppp->crdTrue[i] = 0.0;
	for (i = 0; i < 3; i++) ppp->rr[i] = 0.0;

	ppp->chMsg[0] = '\0';
	ppp->chTime[0] = '\0';

	ppp->ofileName_ful[0] = '\0';
	ppp->ofileName[0] = '\0';
	ppp->obsDir[0] = '\0';
	ppp->obsExt[0] = '\0';

	ppp->nEpoch = 0;
	ppp->iEpoch = 0;
	ppp->iObsu = 0;
	ppp->revs = 0;
	if (ppp->solf) free(ppp->solf);
	ppp->solf = NULL;
	if (ppp->solb) free(ppp->solb);
	ppp->solb = NULL;
	ppp->iSolf = 0;
	ppp->iSolb = 0;

	ppp->dintv = 0.0;
	ppp->delEp = 0;
	ppp->sample = 0.0;
	ppp->zhd = 0.0;

	ppp->clkJump = 0;

	for (i = 0; i < MAXSAT; i++) {
		ppp->ssat_Ex[i] = ssat_EX0;
	}

	ppp->prcOpt_Ex.bElevCheckEx = 0;

	ppp->prcOpt_Ex.tropMF = TROPMF_GMF;

	ppp->prcOpt_Ex.ionopnoise = 1;  //0: static  1: random walk  2:white noise
	ppp->prcOpt_Ex.ion_const = 0;   //0: off     1: on

	//cycle slip set
	ppp->prcOpt_Ex.csThresGF = 0.0;
	ppp->prcOpt_Ex.csThresMW = 0.0;
	ppp->prcOpt_Ex.bUsed_gfCs = 0;
	ppp->prcOpt_Ex.bUsed_mwCs = 0;

	ppp->prcOpt_Ex.tPrcUnit = 0.0;

	//measurement error ratio
	ppp->prcOpt_Ex.errRatioGLO = 100.0;
	ppp->prcOpt_Ex.errRatioBDS = 100.0;
	ppp->prcOpt_Ex.errRatioGAL = 100.0;
	ppp->prcOpt_Ex.errRatioQZS = 100.0;

	//process-noise std for random walk new
	ppp->prcOpt_Ex.prn_iono = 0.001;  //m/sqrt(s)

	//time set
	ppp->prcOpt_Ex.bTsSet = 0;
	ppp->prcOpt_Ex.bTeSet = 0;
	ppp->tNow = gt0;
	ppp->t_30min = gt0;
	ppp->isb_30min = 0.0;
	ppp->sowNow = 0.0;

	//for SPP
	ppp->nBadEpSPP = 0;
	ppp->bOKSPP = 1;

	for (i = 0; i < MAXSAT; i++) {
		ppp->sFlag[i].sys = satsys(i + 1, &ppp->sFlag[i].prn);
		satno2id(i + 1, ppp->sFlag[i].id);
	}

	for (i = 0; i <= MAXPRNGLO; i++) ppp->fnGlo[i] = 0;
}

//read configure file
static void readcfgFile(char file[], prcopt_t* prcopt, solopt_t* solopt, filopt_t* filopt)
{
	/* 局部变量定义 ===================================================================== */
	FILE* fp;						// 读cfg文件指针
	int i, j, k, n;					// 循环遍历变量
	double dt[6] = { 0.0 };			// 时间变量[YYYY/MM/DD hh:mm:ss.s]
	char* p, line[MAXCHARS];		// 读cfg字符指针，每行字符串
	int debug = 1;					// debug指示符
	/* ================================================================================== */

	if ((fp = fopen(file, "r")) == NULL) {
		return;
	}

	// processing options
	prcopt->nf = 2;						// (1:L1 2:L1+L2, 3:L1+L2+L5)
	prcopt->sateph = 1;					// (0:brdc, 1:precise, 2:brdc+sbas, 3:brdc+ssrapc, 4:brdc+ssrcom)
	prcopt->ionoopt = 2;			    // (0:off, 1:brdc, 2:IF12, 3:UC1, 4:UC12, 5:ion-tec)
	prcopt->niter = 1;
	prcopt->maxinno = 30.0;
	prcopt->dynamics = 0;				// (0:none, 1:velociy, 2:accel)
	strcpy(prcopt->anttype, "*");

	// solution options
	solopt->times = 0;					// (0:gpst, 1:utc, 2:jst)
	solopt->timeu = 3;					// time digits under decimal point 小数位
	solopt->timef = 1;				    // (0:tow, 1:hms)
	solopt->height = 0;					// (0:ellipsoidal, 1:geodetic)
	solopt->solstatic = 0;				// (0:all, 1:single)
	solopt->outhead = 0;				// (0:no, 1:yes)
	strcpy(solopt->sep, "");

	while (!feof(fp)) {
		line[0] = '\0';
		fgets(line, MAXCHARS, fp);

		trimSpace(line);
		if (strlen(line) <= 2) continue;
		if (line[0] == '#') continue;

		p = strchr(line, '=');

		// obs开始时间(0:from obs  1:from inp)
		if (strstr(line, "start_time")) {
			for (i = 0; i < 6; i++) dt[i] = 0.0;
			sscanf(p + 1, "%d %lf/%lf/%lf %lf:%lf:%lf", &j, dt + 0, dt + 1, dt + 2, dt + 3, dt + 4, dt + 5);
			PPP_Glo.prcOpt_Ex.bTsSet = j == 1 ? 1 : 0;
			if (PPP_Glo.prcOpt_Ex.bTsSet) PPP_Glo.prcOpt_Ex.ts = epoch2time(dt);
			if (debug) printf("start_time: %d %4.0f/%2.0f/%2.0f %2.0f:%2.0f:%4.1f %d %f\n",
				j, dt[0], dt[1], dt[2], dt[3], dt[4], dt[5], (int)PPP_Glo.prcOpt_Ex.ts.time, PPP_Glo.prcOpt_Ex.ts.sec);
		}
		// obs结束时间(0:from obs  1:from inp)
		else if (strstr(line, "end_time")) {
			for (i = 0; i < 6; i++) dt[i] = 0.0;
			sscanf(p + 1, "%d %lf/%lf/%lf %lf:%lf:%lf", &j, dt + 0, dt + 1, dt + 2, dt + 3, dt + 4, dt + 5);
			PPP_Glo.prcOpt_Ex.bTeSet = j == 1 ? 1 : 0;
			if (PPP_Glo.prcOpt_Ex.bTeSet) PPP_Glo.prcOpt_Ex.te = epoch2time(dt);
			if (debug) printf("end_time: %d %4.0f/%2.0f/%2.0f %2.0f:%2.0f:%4.1f %d %f\n",
				j, dt[0], dt[1], dt[2], dt[3], dt[4], dt[5], (int)PPP_Glo.prcOpt_Ex.te.time, PPP_Glo.prcOpt_Ex.te.sec);
		}
		// 输入频率数
		else if (strstr(line, "inpfrq")) {
			sscanf(p + 1, "%d ", &prcopt->nf);
			if (debug) printf("inpfrq: %d\n", prcopt->nf);
		}
		// 定位模式(0:spp  6:ppp_kinematic  7:ppp_static)
		else if (strstr(line, "posmode")) {
			sscanf(p + 1, "%d", &prcopt->mode);
			if (debug) printf("posmode: %d\n", prcopt->mode);
		}
		// 解算模式(0:forward  1:backward  2:combined-bf  3:combined-fb)
		else if (strstr(line, "soltype")) {
			sscanf(p + 1, "%d", &prcopt->soltype);
			if (debug) printf("soltype: %d\n", prcopt->soltype);
		}
		// 导航系统(1:gps  4:glo  5:gps+glo  8:gal  16:qzs  32:bds)
		else if (strstr(line, "navsys")) {
			sscanf(p + 1, "%d", &prcopt->navsys);
			if (debug) printf("navsys: %d\n", prcopt->navsys);
		}
		// 系统间误差模型(0:off  1:time constant  2:piece-wise constant  3:random walk process  4:white noise process)
		else if (strstr(line, "gnsisb")) {
			sscanf(p + 1, "%d ", &prcopt->gnsisb);
			if (debug) printf("gnsisb: %d\n", prcopt->gnsisb);
		}
		// GLONASS不同频道间误差
		// 0:off  
		// 1:linear function of frequency number  
		// 2:quadratic polynomial function of frequency number
		// 3:isb+icb every sat 
		// 4:isb+icb every frq
		else if (strstr(line, "gloicb")) {
			sscanf(p + 1, "%d ", &prcopt->gloicb);
			if (debug) printf("gloicb: %d\n", prcopt->gloicb);
		}
		// obs最大重置数
		else if (strstr(line, "maxout")) {
			sscanf(p + 1, "%d", &prcopt->maxout);
			if (debug) printf("maxout: %d\n", prcopt->maxout);
		}
		//
		else if (strstr(line, "sampleprc")) {
			sscanf(p + 1, "%lf", &PPP_Glo.prcOpt_Ex.tPrcUnit);
			if (debug) printf("sampleprc: %f\n", PPP_Glo.prcOpt_Ex.tPrcUnit);
		}
		// 卫星截至仰角
		else if (strstr(line, "minelev")) {
			sscanf(p + 1, "%lf", &prcopt->elmin);
			if (debug) printf("minelev: %f\n", prcopt->elmin);
			prcopt->elmin *= D2R;
		}
		// GF周跳检测阈值(m)
		else if (strstr(line, "cycleslip_GF")) {
			sscanf(p + 1, "%d %lf", &j, &PPP_Glo.prcOpt_Ex.csThresGF);
			PPP_Glo.prcOpt_Ex.bUsed_gfCs = j == 1 ? 1 : 0;
			if (debug) printf("cycleslip_GF: %d %f %d\n", j, PPP_Glo.prcOpt_Ex.csThresGF, PPP_Glo.prcOpt_Ex.bUsed_gfCs);
		}
		// MW周跳检测阈值(c)
		else if (strstr(line, "cycleslip_MW")) {
			sscanf(p + 1, "%d %lf", &j, &PPP_Glo.prcOpt_Ex.csThresMW);
			PPP_Glo.prcOpt_Ex.bUsed_mwCs = j == 1 ? 1 : 0;
			if (debug) printf("cycleslip_MW: %d %f %d\n", j, PPP_Glo.prcOpt_Ex.csThresMW, PPP_Glo.prcOpt_Ex.bUsed_mwCs);
		}
		// 对流层估计模式(0:off  1:saas  3:ztd-est  4:ztdgrad-est)
		else if (strstr(line, "tropopt")) {
			sscanf(p + 1, "%d", &prcopt->tropopt);
			if (debug) printf("tropopt: %d\n", prcopt->tropopt);
		}
		// 对流层投影函数(0:nmf  1:gmf)
		else if (strstr(line, "tropmf")) {
			sscanf(p + 1, "%d", &PPP_Glo.prcOpt_Ex.tropMF);
			if (debug) printf("tropmf: %d\n", PPP_Glo.prcOpt_Ex.tropMF);
		}
		// 电离层估计模式(0:off  1:brdc  2:IF12  3:UC1  4:UC12  5:ion-tec)
		else if (strstr(line, "ionoopt")) {
			sscanf(p + 1, "%d", &prcopt->ionoopt);
			if (debug) printf("ionoopt: %d\n", prcopt->ionoopt);
		}
		// 电离层噪声模型(0: static  1: random walk  2: random walk (new)  3:white noise)
		else if (strstr(line, "ionopnoise")) {
			sscanf(p + 1, "%d", &PPP_Glo.prcOpt_Ex.ionopnoise);
			if (debug) printf("ionopnoise: %d\n", PPP_Glo.prcOpt_Ex.ionopnoise);
		}
		// 电离层约束(0:off  1:on)
		else if (strstr(line, "ionconstraint")) {
			sscanf(p + 1, "%d", &PPP_Glo.prcOpt_Ex.ion_const);
			if (debug) printf("ionconstraint: %d\n", PPP_Glo.prcOpt_Ex.ion_const);
		}
		// 潮汐校正(0:off  1:stl  2:otl  4:ptl  7:stl+otl+ptl)
		else if (strstr(line, "tidecorr")) {
			sscanf(p + 1, "%d", &prcopt->tidecorr);
			if (debug) printf("tidecorr: %d\n", prcopt->tidecorr);
		}
		// GPS 误差因子
		else if (strstr(line, "errratio(P/L GPS)")) {
			sscanf(p + 1, "%lf", prcopt->err + 0);
			if (debug) printf("errratio(P/L GPS): %f\n", prcopt->err[0]);
		}
		// GLONASS 误差因子
		else if (strstr(line, "errratio(P/L GLO)")) {
			sscanf(p + 1, "%lf", &PPP_Glo.prcOpt_Ex.errRatioGLO);
			if (debug) printf("errratio(P/L GLO): %f\n", PPP_Glo.prcOpt_Ex.errRatioGLO);
		}
		// BDS 误差因子
		else if (strstr(line, "errratio(P/L BDS)")) {
			sscanf(p + 1, "%lf", &PPP_Glo.prcOpt_Ex.errRatioBDS);
			if (debug) printf("errratio(P/L BDS): %f\n", PPP_Glo.prcOpt_Ex.errRatioBDS);
		}
		// GAL 误差因子
		else if (strstr(line, "errratio(P/L GAL)")) {
			sscanf(p + 1, "%lf", &PPP_Glo.prcOpt_Ex.errRatioGAL);
			if (debug) printf("errratio(P/L GAL): %f\n", PPP_Glo.prcOpt_Ex.errRatioGAL);
		}
		// QZS 误差因子
		else if (strstr(line, "errratio(P/L QZS)")) {
			sscanf(p + 1, "%lf", &PPP_Glo.prcOpt_Ex.errRatioQZS);
			if (debug) printf("errratio(P/L QZS): %f\n", PPP_Glo.prcOpt_Ex.errRatioQZS);
		}
		// 载波误差(m)
		else if (strstr(line, "errmeas(L)")) {
			sscanf(p + 1, "%lf", prcopt->err + 1);
			if (debug) printf("errmeas(L): %f\n", prcopt->err[1]);
			prcopt->err[2] = prcopt->err[1];
		}
		// 模糊度噪声(m/sqrt(s))
		else if (strstr(line, "prcNoise(AMB)")) {
			sscanf(p + 1, "%lf", prcopt->prn + 0);
			if (debug) printf("prcNoise(AMB): %14.10f\n", prcopt->prn[0]);
		}
		// 电离层噪声(m/sqrt(s))
		else if (strstr(line, "prcNoise(ION)")) {
			sscanf(p + 1, "%lf", prcopt->prn + 1);
			if (debug) printf("prcNoise(ION): %f\n", prcopt->prn[1]);
		}
		// 电离层噪声（新）(m/sqrt(s))
		else if (strstr(line, "prcNoise(ION_GF)")) {
			sscanf(p + 1, "%lf", &PPP_Glo.prcOpt_Ex.prn_iono);
			if (debug) printf("prcNoise(ION_GF): %f\n", PPP_Glo.prcOpt_Ex.prn_iono);
		}
		// 对流层噪声(m/sqrt(s))
		else if (strstr(line, "prcNoise(ZTD)")) {
			sscanf(p + 1, "%lf", prcopt->prn + 2);
			if (debug) printf("prcNoise(ZTD): %f\n", prcopt->prn[2]);
		}
		// ？？输出时间格式(0:sssss.s,1:yyyy/mm/dd hh:mm:ss.s)
		else if (strstr(line, "outtime")) {
			sscanf(p + 1, "%d", &solopt->timef);
			if (debug) printf("outtime: %d\n", solopt->timef);
		}
		// ？？输出经纬度格式(0:ddd.ddd,1:ddd mm ss)
		else if (strstr(line, "outdeg")) {
			sscanf(p + 1, "%d", &solopt->degf);
			if (debug) printf("outdeg: %d\n", solopt->degf);
		}
		// 输出文件夹
		else if (strstr(line, "outdir")) {
			if (strlen(PPP_Glo.outFolder) <= 0) {
				sscanf(p + 1, "%[^,]", PPP_Glo.outFolder);
				trimSpace(PPP_Glo.outFolder);
				if (debug) printf("outdir: %s\n", PPP_Glo.outFolder);
			}
		}
		// 输出文件数量
		else if (strstr(line, "output")) {
			sscanf(p + 1, "%d", &n);
			for (i = 0; i < n; i++) {
				line[0] = '\0';
				fgets(line, MAXSTRPATH, fp);
				trimSpace(line);
				if (strlen(line) <= 2) continue;
				if (line[0] == '#') continue;

				p = strchr(line, '=');

				for (j = 0; j < MAXOUTFILE; j++) {
					if (strlen(outtype[j]) < 1) continue;
					if (strstr(line, outtype[j]) && strstr(line, "=")) {
						k = 0;
						sscanf(p + 1, "%d", &k);
						solopt->fpout[j] = 1 == k ? 1 : 0;
						break;
					}
				}
			}
		}
		/*else if (strstr(line,"input")) {
			sscanf(p+1,"%d",&n);
			for (i=0;i<n;i++) {
				line[0]='\0';
				fgets(line,MAXSTRPATH,fp);
				sscanf(p+1,"%s",tmpline);
				trimSpace(tmpline);
				ext=strrchr(tmpline,'.');
				if (strstr(ext+1,"atx")) {
					strcpy(filopt->antf,tmpline);
					if (debug) printf("ant: %s\n",filopt->antf);
				}
				else if (strstr(ext+1,"DCB")) {
					if (strstr(line,"p1c1dcb")) {
						strcpy(filopt->p1c1dcbf,tmpline);
						if (debug) printf("p1c1dcb: %s\n",filopt->p1c1dcbf);
					}
					else if (strstr(line,"p2c2dcb")) {
						strcpy(filopt->p2c2dcbf,tmpline);
						if (debug) printf("p2c2dcb: %s\n",filopt->p2c2dcbf);
					}
					else if (strstr(line,"p1p2dcb")) {
						strcpy(filopt->p1p2dcbf,tmpline);
						if (debug) printf("p1p2dcb: %s\n",filopt->p1p2dcbf);
					}
				}
				else if (strstr(ext+1,"erp")) {
					strcpy(filopt->eopf,tmpline);
					if (debug) printf("eop: %s\n",filopt->eopf);
				}
				else if (strstr(ext+1,"blq")) {
					strcpy(filopt->blqf,tmpline);
					if (debug) printf("blq: %s\n",filopt->blqf);
				}
				else if (strstr(ext+1,"crd")) {
					strcpy(filopt->stapos,tmpline);
					if (debug) printf("stapos: %s\n",filopt->stapos);
				}
			}
		}*/
	}
	fclose(fp);

	if (prcopt->mode == PMODE_SINGLE) {
		prcopt->tropopt = TROPOPT_SAAS;
		prcopt->sateph = EPHOPT_BRDC;
	}

	PPP_Glo.prcOpt_Ex.navSys  = prcopt->navsys;
	PPP_Glo.prcOpt_Ex.posMode = prcopt->mode;
	PPP_Glo.prcOpt_Ex.solType = prcopt->soltype;
};

//auto match input files
static void getFopt_auto(char obsfile[], char dir[], gtime_t ts, gtime_t te, const prcopt_t popt, const solopt_t sopt, filopt_t* fopt)
{
	/* 局部变量定义 ===================================================================== */
	int i  = 0;												  // 循环遍历变量
	int ni = 0;												  // 输入文件计数
	int n;													  // 文件计数器（临时）
	char tmp[MAXSTRPATH] = { '\0' };						  // 每行写入字符串
	char sep = (char)FILEPATHSEP;                             // 分隔符"\\"
	const int navsys[] = { SYS_GPS,SYS_GLO,SYS_CMP,SYS_GAL }; // 导航系统索引
	/* ================================================================================== */

	/* 1.obs file 输入obs文件名【观测文件】---------------------------------------------- */
	strcpy(fopt->inf[ni++], obsfile);

	/* 2.sinex file 查找snx文件，并遍历snx中对应站真值【基站真值】----------------------- */
	findSnxFile(ts, te, dir, fopt->snxf);
	if (PPP_Glo.crdTrue[0] * PPP_Glo.crdTrue[1] * PPP_Glo.crdTrue[2] == 0.0) {
		getCoord_snx(fopt->snxf, PPP_Glo.sitName, PPP_Glo.crdTrue);
	}

	/* 3.nav file 查找nav文件【星历文件】------------------------------------------------ */
	n = 0;
	// 3.1 先找单系统
	for (i = 0; i < 4; i++) {
		if (!(navsys[i] & popt.navsys)) continue;
		n += findNavFile(ts, te, obsfile, fopt->inf, ni + n, navsys[i]);
	}
	// 3.2 若单系统没有，找混合系统
	if (n == 0) {
		n += findNavFile_p(ts, te, obsfile, fopt->inf, ni);
	}
	// 3.3 若都没有，报错
	if (n == 0) {
		printf("*** ERROR: navigation files NOT found in obs dir!\n");
		return;
	}
	ni += n;

	/* 4.查找Clk文件【精密时钟】--------------------------------------------------------- */
	n = findClkFile(ts, te, dir, fopt->inf, ni);
	ni += n;

	/* 5.查找Sp3文件【精密星历】--------------------------------------------------------- */
	ni += findSp3File(ts, te, dir, fopt->inf, ni);

	/* 6.查找Sp3文件【精密星历】--------------------------------------------------------- */
	findP1P2DcbFile(ts, te, dir, fopt->p1p2dcbf);
	findP1C1DcbFile(ts, te, dir, fopt->p1c1dcbf);
	findP2C2DcbFile(ts, te, dir, fopt->p2c2dcbf);
	if (popt.navsys & SYS_CMP || popt.navsys & SYS_GAL) findMGEXDcbFile(ts, te, dir, fopt->mgexdcbf);

	if (popt.tidecorr & 4) findErpFile(ts, te, dir, fopt->eopf);

	if (popt.ionoopt == IONOOPT_TEC || ((popt.ionoopt == IONOOPT_UC1 || popt.ionoopt == IONOOPT_UC12) &&
		PPP_Glo.prcOpt_Ex.ion_const))
		findIonFile(ts, te, dir, fopt->ionf);

	if (popt.tidecorr & 2) findBlqFile(ts, te, dir, fopt->blqf);

	findAtxFile(ts, te, dir, fopt->antf);

	if (PPP_Glo.crdTrue[0] * PPP_Glo.crdTrue[1] * PPP_Glo.crdTrue[2] == 0.0) { // 获取站的真值坐标
		sprintf(tmp, "%s%csite.crd", PPP_Glo.obsDir, sep);
		if (access(tmp, 0) != -1) getCoord_i(tmp, PPP_Glo.sitName, PPP_Glo.crdTrue);
	}
	if (PPP_Glo.crdTrue[0] * PPP_Glo.crdTrue[1] * PPP_Glo.crdTrue[2] == 0.0)
		PPP_Glo.crdTrue[0] = PPP_Glo.crdTrue[1] = PPP_Glo.crdTrue[2] = 0.0;

	//for output files
	sprintf(tmp, "%s%c%s%c%s", PPP_Glo.obsDir, sep, PPP_Glo.outFolder, sep, PPP_Glo.ofileName_ful);
	for (i = 0; i < MAXOUTFILE; i++) {
		if (strlen(outtype[i]) > 0 && sopt.fpout[i])
			sprintf(fopt->outf[i], "%s.%s", tmp, outtype[i]);
	}
}

//get informations of start and end time etc.
static int preProc(char* file, procparam_t* pparam, gtime_t* ts, gtime_t* te)
{
	/* 局部变量定义 ===================================================================== */
	int i;										// 循环体遍历变量
	double delta[3];						    // 偏移量（H，E，N）
	char anttype[100], rcvtype[40];			    // 天线类型、接收机类型
	/* ================================================================================== */

	pparam->filopt = filopt_default;
	pparam->solopt = solopt_default;
	pparam->prcopt = prcopt_default;
	for (i = 0; i < MAXINFILE; i++)  pparam->filopt.inf[i] = NULL;
	for (i = 0; i < MAXOUTFILE; i++) pparam->filopt.outf[i] = NULL;

	// 1.初始化PPP_Glo结构体
	initGlobal(&PPP_Glo);

	// 获取obs信息
	getObsInfo(file, anttype, rcvtype, delta, ts, te, PPP_Glo.sitName, PPP_Glo.ofileName, PPP_Glo.ofileName_ful, PPP_Glo.obsExt);

	xStrMid(PPP_Glo.prcOpt_Ex.rcvType, 0, 20, rcvtype);  // 接收机型号
	xStrMid(pparam->prcopt.anttype, 0, 20, anttype);	 // 天线型号
	//antdel: E,N,U
	pparam->prcopt.antdel[0] = delta[1];
	pparam->prcopt.antdel[1] = delta[2];
	pparam->prcopt.antdel[2] = delta[0];

	//allocation for 'inf' and 'outf' of 'filopt' 分配内存空间
	for (i = 0; i < MAXINFILE; i++) {
		if (!(pparam->filopt.inf[i] = (char*)malloc(MAXSTRPATH))) {
			for (i--; i >= 0; i--) free(pparam->filopt.inf[i]);
			return 0;
		}
		else
			pparam->filopt.inf[i][0] = '\0';
	}

	for (i = 0; i < MAXOUTFILE; i++) {
		if (!(pparam->filopt.outf[i] = (char*)malloc(MAXSTRPATH))) {
			for (i--; i >= 0; i--) free(pparam->filopt.outf[i]);
			return 0;
		}
		else
			pparam->filopt.outf[i][0] = '\0';
	}

	return 1;
}

static void postProc(procparam_t pparam)
{
	int i;

	for (i = MAXINFILE - 1; i >= 0; i--) {
		if (pparam.filopt.inf[i])
			free(pparam.filopt.inf[i]);
	}

	for (i = MAXOUTFILE - 1; i >= 0; i--) {
		if (pparam.filopt.outf[i])
			free(pparam.filopt.outf[i]);
	}
}

//processing single ofile
extern void procOneFile(char file[], char cfgfile[], int iT, int iN)
{
	/* 局部变量定义 ===================================================================== */
	procparam_t pparam;		// 后处理处理结构体：[1] 处理选项、[2] 解算选项、[3] 文件选项
	gtime_t ts = { 0 };		// obs开始时间
	gtime_t te = { 0 };		// obs结束之间
	long t1, t2;			// t1：程序开始时间 t2：程序结束时间
	/* ================================================================================== */

	t1 = clock();

	// 1.对pparam做预处理
	preProc(file, &pparam, &ts, &te);

	printf(" * Processing the %dth", iN);
	if (iT > 0) printf("/%d", iT);
	printf(" ofile: %s\n", PPP_Glo.ofileName_ful);

	// 2.read configure file
	readcfgFile(cfgfile, &pparam.prcopt, &pparam.solopt, &pparam.filopt);

	// single-, dual- or triple-frequency?
	if (pparam.prcopt.ionoopt == IONOOPT_IF12 || pparam.prcopt.ionoopt == IONOOPT_UC1) {
		if (pparam.prcopt.nf != 1) {
			printf("*** ERROR: Number of frequencies Error! Please set inpfrq=1.\n");
			return;
		}
	}
	if (pparam.prcopt.ionoopt == IONOOPT_UC12) {
		if (pparam.prcopt.nf != 2) {
			printf("*** ERROR: Number of frequencies Error! Please set inpfrq=2.\n");
			return;
		}
	}

	// processing time set
	if (!PPP_Glo.prcOpt_Ex.bTsSet) PPP_Glo.prcOpt_Ex.ts = ts;
	else if (timediff(ts, PPP_Glo.prcOpt_Ex.ts) > 0) PPP_Glo.prcOpt_Ex.ts = ts;
	if (!PPP_Glo.prcOpt_Ex.bTeSet) PPP_Glo.prcOpt_Ex.te = te;
	else if (timediff(te, PPP_Glo.prcOpt_Ex.te) < 0) PPP_Glo.prcOpt_Ex.te = te;

	// 3.automatically matches the corresponding files
	getFopt_auto(file, PPP_Glo.obsDir, ts, te, pparam.prcopt, pparam.solopt, &pparam.filopt);

	// post processing positioning 后处理入口！！！
	gampPos(PPP_Glo.prcOpt_Ex.ts, PPP_Glo.prcOpt_Ex.te, 0.0, 0.0, &pparam.prcopt, &pparam.solopt, &pparam.filopt);

	postProc(pparam);

	t2 = clock();

	sprintf(PPP_Glo.chMsg, " * The program runs for %6.3f seconds\n%c", (double)(t2 - t1) / CLOCKS_PER_SEC, '\0');
	outDebug(OUTWIN, OUTFIL, 0);
	printf("/*****************************  OK  *****************************/\n\n\n");

	if (PPP_Glo.outFp[OFILE_DEBUG]) {
		fclose(PPP_Glo.outFp[OFILE_DEBUG]);
		PPP_Glo.outFp[OFILE_DEBUG] = NULL;
	}
}

// batch processing ofiles
extern void batchProc(char folder[], char cfgfile[])
{
	DIR* dir;
	struct dirent* file;
	char filepath[MAXSTRPATH] = { '\0' }, * ext;
	char sep = (char)FILEPATHSEP;
	int i = 1;

	if (!(dir = opendir(folder))) {
		printf("*** ERROR: open obsdir failed, please check it!\n");
		return;
	}

	while ((file = readdir(dir)) != NULL) {
		if (strncmp(file->d_name, ".", 1) == 0) continue;  //skip ".",".." for linux
		if (!(ext = strrchr(file->d_name, '.'))) continue;
		if (!strstr(ext + 3, "o")) continue;
		sprintf(filepath, "%s%c%s", folder, sep, file->d_name);
		procOneFile(filepath, cfgfile, 0, i++);
	}
	closedir(dir);
}