#include <opencv2/imgproc/imgproc_c.h>  
#include <opencv2/legacy/legacy.hpp>  
#include "opencv2/highgui/highgui.hpp"  
#include<opencv2\opencv.hpp>  
#include<iostream>  
#include <stdio.h>
#include <gmp.h>
#include <windows.h>
using namespace std;
using namespace cv;
static float B[3][3] = { { 1, 2, 3 }, { 0, 1, 4 }, { 5, 6, 0 } };//测试成功
static float B1[2][2] = { { 3, 4 }, { 4, 5 } };
/*以p密文形式保存明文点*/
static void printHelper(double x, double y){
	//cout << "(x,y): (" << x << "," << y << ")" << "\n";
}

CV_INLINE void printMat(CvMat* matrix){
	for (int i = 0; i < matrix->rows; i++)//行   
	{
		for (int j = 0; j < matrix->cols; j++)
		{
			cout << (double)(cvGetReal2D(matrix, i, j)) << "\n";
		}
	}
}

static string mat2String(CvMat* matrix){
	string res = "";
	for (int i = 0; i < matrix->rows; i++)//行  
	{
		for (int j = 0; j < matrix->cols; j++)
		{
			 res = res + to_string(cvGetReal2D(matrix, i, j));
		}
	}
	return res;
}

//static string uchar2String(unsigned char* c){
//	string res = "";
//	for (int i = 0; i < 16; i++){
//		res = res + c[i];
//	}
//	return res;
//}
/*gen random key*/
static string ase128_random_key(){
	string res = "";
	for (int i = 0; i < 16; i++){
		int c = rand() % 128;
		res = res + (char)c;
	}
	return res;
}

unsigned char* string2Uchar(string str){
	unsigned char* res = new unsigned char[str.size()];
	for (int i = 0; i < str.size(); i++){
		char c = str[i];
		unsigned int intC = c;
		res[i] = intC;
	}
	return res;
}

static char* ltos(long l)
{
	char c[32];
	sprintf(c, "%ld", l);
	return c;
}
static string or(string s1, string s2){
	string res = "";
	int s1Size = s1.size();
	int s2Size = s2.size();
	if (s1Size < s2Size){
		s2 = s2.substr(0, s1Size);
	}
	if (s1.size() > s2.size()){
		int overSize = s1Size - s2Size;
		for (int i = 0; i < overSize; i++){
			s2 = s2 + "0";
		}
	}

	if (s1.size() == s2.size()){
		for (int i = 0; i < s1.size(); i++){
			char c = s1[i] ^ s2[i];
			res = res + c;
		}
	}
	if (res == ""){
		cout << "fs";
	}
	return res;
}

static CvMat* CvPoint2CvMatS(CvPoint2D32f point){
	CvMat* res = cvCreateMat(3, 1, CV_32FC1);
	CV_MAT_ELEM(*res, double, 0, 0) = point.x * 3;
	CV_MAT_ELEM(*res, double, 1, 0) = point.y * 3;
	CV_MAT_ELEM(*res, double, 2, 0) = sqrt(point.x * point.x + point.y * point.y) * 3;
	return res;
}
static CvMat* CvPoint2CvMatOfP(CvPoint2D32f point){
	float p[3][1] = { 0 };
	p[0][0] = point.x;
	p[1][0] = point.y;
	p[2][0] = -0.5 * (point.x * point.x + point.y * point.y);
	CvMat M = cvMat(3, 3, CV_32FC1, B);
	CvMat P = cvMat(3, 1, CV_32FC1, p);
	//cvTranspose(&M, MT);
	CvMat* res = cvCreateMat(3, 1, CV_32FC1);//这样是没有数据分配的
	cvGEMM(&M, &P, 1, &P, 0, res);//3 * 3 * 3 * 1
	return res;
}
static CvMat* CvPoint2CvMatOfM(CvPoint2D32f point){
	float p[3][1] = { 0 };
	p[0][0] = point.x;
	p[1][0] = point.y;
	p[2][0] = 0;
	CvMat M = cvMat(3, 3, CV_32FC1, B);
	CvMat P = cvMat(3, 1, CV_32FC1, p);
	CvMat* res = cvCreateMat(3, 1, CV_32FC1);
	
	cvGEMM(&M, &P, 1, &M, 0, res, 1);
	return res;
}
/*以q密文形式保存明文点*/
static CvMat* CvPoint2CvMatOfQ(CvPoint2D32f point){
	float q[3][1] = { 0 };
	q[0][0] = 3 * point.x;
	q[1][0] = 3 * point.y;
	q[2][0] = 3 * 1;
	CvMat M = cvMat(3, 3, CV_32FC1, B);

	CvMat* MI = cvCreateMat(3, 3, CV_32FC1);
	CvMat Q = cvMat(3, 1, CV_32FC1, q);
	cvInvert(&M, MI);
	CvMat* res = cvCreateMat(3, 1, CV_32FC1);//这样是没有数据分配的

	cvGEMM(MI, &Q, 1, MI, 0, res, 1);//3 * 3 * 3 * 1

	return res;
}

static bool CvCompare(CvMat* src, CvMat* dst){
	if (src->rows == dst->rows & src->cols == dst->cols){
		for (int i = 0; i < src->rows; i++){
			for (int j = 0; j < src->cols; j++){
				if ((double)(cvGetReal2D(src, i, j)) != (double)(cvGetReal2D(dst, i, j))){
					return false;
				}
			}
		}
		return true;
	}
	return false;
}

//得到当前时间-毫秒
static long getCurrentTime()
{
	SYSTEMTIME sys;
	GetLocalTime(&sys);
	return sys.wMilliseconds;
}

static string int2str(const int &int_temp )
{
	stringstream stream;
	stream << int_temp;
	return stream.str();   //此处也可以用 stream>>string_temp  
}

static int str2int(const string &string_temp)
{
	int int_temp;
	stringstream stream(string_temp);
	stream >> int_temp;
	return int_temp;
}
static const char* double2Char(double d){
	ostringstream os;

	if (os << d){
		string str = os.str();
		return str.c_str();
	}
		
	return "invalid conversion";
}


static int check(CvMat* ab, CvMat* ac, CvMat* ap){
	int scale = 10000000;
	double a1 = cvGetReal2D(ab, 0, 0)*scale;
	double a2 = cvGetReal2D(ab, 1, 0)*scale;
	double a3 = cvGetReal2D(ab, 2, 0)*scale;
	const char* a1s = double2Char(a1);
	const char* a2s = double2Char(a2);
	const char* a3s = double2Char(a3);

	double b1 = cvGetReal2D(ac, 0, 0)*scale;
	double b2 = cvGetReal2D(ac, 1, 0)*scale;
	double b3 = cvGetReal2D(ac, 2, 0)*scale;
	const char* b1s = double2Char(b1);
	const char* b2s = double2Char(b2);
	const char* b3s = double2Char(b3);

	mpz_t f_a1, f_a2, f_a3;

	//test
	//char *char_rand = new char[2];
	//char_rand[0] = '1'; 
	//char_rand[1] = '0';
	//char_rand[2] = '\0';
	////char_rand[1] = "2";
	//mpz_t mpzPrime;
	//mpz_init(mpzPrime);
	//mpz_set_str(mpzPrime, char_rand, 10);
	//gmp_printf("p=%Zd\n", mpzPrime);
	//mpz_t p;
	//mpz_init(p);
	//CreateBigPrime(p, 1024);

	/*mpz_t f;
	mpz_init_set_str(f, "1000.00", 10);
	int n = 0;*/
	//test
	mpz_t zero;
	mpz_init_set_d(zero, 0);
	//mpz_init_set_str(f_a1, "1000000", 10);
	//int j = mpz_cmp(f_a1, zero);
	mpz_init_set_d(f_a1, a1);//初始化
	mpz_init_set_d(f_a2, a2);
	mpz_init_set_d(f_a3, a3);
	//mpz_mul(zero, f_a1, f_a1);
	//gmp_printf("p=%fd\n", f_a1);
	mpz_t f_b1, f_b2, f_b3;

	mpz_init_set_d(f_b1, b1);
	mpz_init_set_d(f_b2, b2);
	mpz_init_set_d(f_b3, b3);


	mpz_t f_c1_temp1, f_c1_temp2, f_c1_temp3, f_c1_temp4;
	mpz_init(f_c1_temp1); mpz_init(f_c1_temp2); mpz_init(f_c1_temp3); mpz_init(f_c1_temp4);

	
	mpz_mul(f_c1_temp1, f_a2, f_b3);
	//gmp_printf("fixed point mpz %Zd\t%Zd = %Zd\n", f_a2, f_a3, f_c1_temp1);
	//long double C1 = a2*b3;
	//long double C2 = a3 * b2;

	//long double c1 = a2 * b3 - a3 * b2;
	//long double c2 = -(a1 * b3 - b1 * a3);
	//long double c3 = a1 * b2 - a2 * b1;


	//ab * ac
	mpz_t f_c1, f_c2, f_c3;
	mpz_init(f_c1); mpz_init(f_c2); mpz_init(f_c3); 
	mpz_mul(f_c1_temp1, f_a2, f_b3);
	//gmp_printf("c1: %Zd\t%Zd = %Zd\n", f_a2, f_b3, f_c1_temp1);
	mpz_mul(f_c1_temp2, f_a3, f_b2);
	//gmp_printf("c1: %Zd\t%Zd = %Zd\n", f_a3, f_b2, f_c1_temp2);
	mpz_sub(f_c1, f_c1_temp1, f_c1_temp2);
	//gmp_printf("_c1: %Zd\t%Zd = %Zd\n", f_c1, f_c1_temp1, f_c1_temp2);

	mpz_mul(f_c1_temp1, f_a1, f_b3);
	//gmp_printf("_c2: %Zd\t%Zd = %Zd\n", f_a1, f_b3, f_c1_temp1);
	mpz_mul(f_c1_temp2, f_b1, f_a3);
	//gmp_printf("_c2: %Zd\t%Zd = %Zd\n", f_b1, f_a3, f_c1_temp2);
	mpz_sub(f_c2, f_c1_temp2, f_c1_temp1);
	//gmp_printf("_c2: %Zd\t%Zd = %Zd\n", f_c2, f_c1_temp2, f_c1_temp1);

	mpz_mul(f_c1_temp1, f_a1, f_b2);
	//gmp_printf("_c3: %Zd\t%Zd = %Zd\n", f_c1_temp1, f_a1, f_b2);
	mpz_mul(f_c1_temp2, f_a2, f_b1);
	//gmp_printf("_c3: %Zd\t%Zd = %Zd\n", f_c1_temp2, f_a2, f_b1);
	mpz_sub(f_c3, f_c1_temp1, f_c1_temp2);
	//gmp_printf("_c3: %Zd\t%Zd = %Zd\n", f_c3, f_c1_temp1, f_c1_temp2);

	b1 = cvGetReal2D(ap, 0, 0)*scale;
	b2 = cvGetReal2D(ap, 1, 0)*scale;
	b3 = cvGetReal2D(ap, 2, 0)*scale;
	b1s = double2Char(b1);
	b2s = double2Char(b2);
	b3s = double2Char(b3);

	/*double _c1 = a2 * b3 - a3 * b2;
	double _c2 = -(a1 * b3 - b1 * a3);
	double _c3 = a1 * b2 - a2 * b1;*/


	mpz_init_set_d(f_b1, b1);
	mpz_init_set_d(f_b2, b2);
	mpz_init_set_d(f_b3, b3);

	mpz_t _f_c1, _f_c2, _f_c3;
	mpz_init(_f_c1); mpz_init(_f_c2); mpz_init(_f_c3);
	mpz_mul(f_c1_temp1, f_a2, f_b3);
//	gmp_printf("_c1: %Zd\t%Zd = %Zd\n", f_a2, f_b3, f_c1_temp1);
	mpz_mul(f_c1_temp2, f_a3, f_b2);
	//gmp_printf("_c1: %Zd\t%Zd = %Zd\n", f_a3, f_b2, f_c1_temp2);
	mpz_sub(_f_c1, f_c1_temp1, f_c1_temp2);
	//gmp_printf("_c1: %Zd\t%Zd = %Zd\n", f_c1_temp1, f_c1_temp2, _f_c1);

	mpz_mul(f_c1_temp1, f_a1, f_b3);
	//gmp_printf("_c2: %Zd\t%Zd = %Zd\n", f_a1, f_b3, f_c1_temp1);
	mpz_mul(f_c1_temp2, f_b1, f_a3);
	//gmp_printf("_c2: %Zd\t%Zd = %Zd\n", f_b1, f_a3, f_c1_temp2);
	mpz_sub(_f_c2, f_c1_temp2, f_c1_temp1);
	//gmp_printf("_c2: %Zd\t%Zd = %Zd\n", f_c1_temp2, f_c1_temp1, _f_c2);

	mpz_mul(f_c1_temp1, f_a1, f_b2);
	//gmp_printf("_c3: %Zd\t%Zd = %Zd\n", f_c1_temp1, f_a1, f_b2);
	mpz_mul(f_c1_temp2, f_a2, f_b1);
	
	mpz_sub(_f_c3, f_c1_temp1, f_c1_temp2);
	//gmp_printf("_c3: %Zd\t%Zd = %Zd\n", _f_c3, f_c1_temp1, f_c1_temp2);

	mpz_t res;
	mpz_init(res);
	mpz_mul(f_c1_temp1, f_c1, _f_c1);
	mpz_mul(f_c1_temp2, f_c2, _f_c2);
	mpz_mul(f_c1_temp3, f_c3, _f_c3);

	mpz_add(f_c1_temp4, f_c1_temp1, f_c1_temp2);

	mpz_add(res, f_c1_temp4, f_c1_temp3);
	
	
	int i = mpz_cmp(res, zero);
	mpz_clear(f_a1);
	mpz_clear(f_a2);
	mpz_clear(f_a3);
	mpz_clear(f_b1);
	mpz_clear(f_b2);
	mpz_clear(f_b3);
	mpz_clear(f_c1);
	mpz_clear(f_c2);
	mpz_clear(f_c3);
	mpz_clear(_f_c1);
	mpz_clear(_f_c2);
	mpz_clear(_f_c3);
	mpz_clear(f_c1_temp1);
	mpz_clear(f_c1_temp2);
	mpz_clear(f_c1_temp3);
	mpz_clear(f_c1_temp4);
	mpz_clear(zero);
	mpz_clear(res);
	
	return i;
}
