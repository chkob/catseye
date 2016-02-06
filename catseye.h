//---------------------------------------------------------
//	Cat's eye
//
//		©2016 Yuichiro Nakada
//---------------------------------------------------------

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define CATS_SIGMOID
//#define CATS_TANH
//#define CATS_SCALEDTANH
//#define CATS_RELU
//#define CATS_ABS

// activation function and derivative of activation function
#ifdef CATS_SIGMOID
// sigmoid function
#define ACTIVATION_FUNCTION(x)		(1.0 / (1.0 + exp(-x)))
#define DACTIVATION_FUNCTION(x)		((1.0-x)*x)	// ((1.0-sigmod(x))*sigmod(x))
#elif defined CATS_TANH
// tanh function
#define ACTIVATION_FUNCTION(x)		(tanh(x))
#define DACTIVATION_FUNCTION(x)		(1.0 - x*x)	// (1.0 - tanh(x)*tanh(x))
#elif defined CATS_SCALEDTANH
// scaled tanh function
#define ACTIVATION_FUNCTION(x)		(1.7159 * tanh(2.0/3.0 * x))
#define DACTIVATION_FUNCTION(x)		((2.0/3.0)/1.7159 * (1.7159-x)*(1.7159+x))
#elif defined CATS_RELU
// rectified linear unit function
#define ACTIVATION_FUNCTION(x)		(x>0 ? x : 0.0)
#define DACTIVATION_FUNCTION(x)		(x>0 ? 1.0 : 0.0)
#elif defined CATS_ABS
// abs function
#define ACTIVATION_FUNCTION(x)		(x / (1.0 + fabs(x)))
#define DACTIVATION_FUNCTION(x)		(1.0 / (1.0 + fabs(x))*(1.0 + fabs(x)))
#else
// identity function (output only)
#define ACTIVATION_FUNCTION(x)		(x)
#define DACTIVATION_FUNCTION(x)		(1.0)
#endif

typedef struct {
	// number of each layer
	int in, hid, out;
	// input layer
//	double *xi1, *xi2, *xi3;
	double *xi2, *xi3;
	// output layer
	double *o1, *o2, *o3;
	// error value
	double *d2, *d3;
	// weights
	double *w1, *w2;
} CatsEye;

/* constructor
 * n_in:  number of input layer
 * n_hid: number of hidden layer
 * n_out: number of output layer */
void CatsEye__construct(CatsEye *this, int n_in, int n_hid, int n_out, char *filename)
{
	FILE *fp;
	if (filename) {
		fp = fopen(filename, "r");
		if (fp==NULL) return;
		fscanf(fp, "%d %d %d\n", &this->in, &this->hid, &this->out);
	} else {
		// unit number
		this->in = n_in;
		this->hid = n_hid;
		this->out = n_out;
	}

	// allocate inputs
//	this->xi1 = malloc(sizeof(double)*(this->in+1));
	this->xi2 = malloc(sizeof(double)*(this->hid+1));
	this->xi3 = malloc(sizeof(double)*this->out);

	// allocate outputs
	this->o1 = malloc(sizeof(double)*(this->in+1));
	this->o2 = malloc(sizeof(double)*(this->hid+1));
	this->o3 = malloc(sizeof(double)*this->out);

	// allocate errors
	this->d2 = malloc(sizeof(double)*(this->hid+1));
	this->d3 = malloc(sizeof(double)*this->out);

	// allocate memories
	this->w1 = malloc(sizeof(double)*(this->in+1)*this->hid);
	this->w2 = malloc(sizeof(double)*(this->hid+1)*this->out);

	if (filename) {
		for(int i=0; i<(this->in+1)*this->hid; i++) {
			fscanf(fp, "%lf ", &this->w1[i]);
		}
		for(int i=0; i<(this->hid+1)*this->out; i++) {
			fscanf(fp, "%lf ", &this->w2[i]);
		}
	} else {
		// initialize weights
		// range depends on the research of Y. Bengio et al. (2010)
		srand((unsigned)(time(0)));
		double range = sqrt(6)/sqrt(this->in+this->hid+2);
		srand((unsigned)(time(0)));
		for (int i=0; i<(this->in+1)*this->hid; i++) {
			this->w1[i] = 2.0*range*rand()/RAND_MAX-range;
		}
		for (int i=0; i<(this->hid+1)*this->out; i++) {
			this->w2[i] = 2.0*range*rand()/RAND_MAX-range;
		}
	}
}

// deconstructor
void CatsEye__destruct(CatsEye *this)
{
	// delete arrays
//	free(this->xi1);
	free(this->xi2);
	free(this->xi3);
	free(this->o1);
	free(this->o2);
	free(this->o3);
	free(this->d2);
	free(this->d3);
	free(this->w1);
	free(this->w2);
}

// caluculate forward propagation of input x
void CatsEye_forward(CatsEye *this, double *x)
{
	// calculation of input layer
//	memcpy(this->xi1, x, this->in*sizeof(double));
	memcpy(this->o1, x, this->in*sizeof(double));
	this->o1[this->in] = 1;

	// caluculation of hidden layer
	for (int j=0; j<this->hid; j++) {
		this->xi2[j] = 0;
		for (int i=0; i<this->in+1; i++) {
			this->xi2[j] += this->w1[i*this->hid+j]*this->o1[i];
		}
		this->o2[j] = ACTIVATION_FUNCTION(this->xi2[j]);
	}
	this->o2[this->hid] = 1;

	// caluculation of output layer
	for (int j=0; j<this->out; j++) {
		this->xi3[j] = 0;
		for (int i=0; i<this->hid+1; i++) {
			this->xi3[j] += this->w2[i*this->out+j]*this->o2[i];
		}
		this->o3[j] = this->xi3[j];
	}
}

/* train: multi layer perceptron
 * x: train data (number of elements is in*N)
 * t: correct label (number of elements is N)
 * N: data size
 * repeat: repeat times
 * eta: learning rate */
void CatsEye_train(CatsEye *this, double *x, int *t, double N, int repeat/*=1000*/, double eta/*=0.1*/)
{
//#pragma omp parallel for
	for (int times=0; times<repeat; times++) {
		double err = 0;
		for (int sample=0; sample<N; sample++) {
			// forward propagation
			CatsEye_forward(this, x+sample*this->in);

			// calculate the error of output layer
			for (int j=0; j<this->out; j++) {
				if (t[sample] == j) {	// classifying
					this->d3[j] = this->o3[j]-1;
				} else {
					this->d3[j] = this->o3[j];
				}
			}
			// update the weights of output layer
			for (int i=0; i<this->hid+1; i++) {
				for (int j=0; j<this->out; j++) {
					this->w2[i*this->out+j] -= eta*this->d3[j]*this->o2[i];
				}
			}
			// calculate the error of hidden layer
			for (int j=0; j<this->hid+1; j++) {
				double tmp = 0;
				for (int l=0; l<this->out; l++) {
					tmp += this->w2[j*this->out+l]*this->d3[l];
				}
				//this->d2[j] = tmp * DACTIVATION_FUNCTION(this->xi2[j]);	// xi2 = z
				this->d2[j] = tmp * DACTIVATION_FUNCTION(this->o2[j]);	// o2 = f(z)
			}
			// update the weights of hidden layer
			for (int i=0; i<this->in+1; i++) {
				for (int j=0; j<this->hid; j++) {
					this->w1[i*this->hid+j] -= eta*this->d2[j]*this->o1[i];
				}
			}

			// calculate the mean squared error
			double mse = 0;
			for (int i=0; i<this->out; i++) {
				mse += 0.5 * (this->d3[i] * this->d3[i]);
			}
			err = 0.5 * (err + mse);
		}
		printf("ephochs %d, mse %f\n", times, err);
//		printf(".");
//		fflush(stdout);
	}
//	printf("\n");
}

// return most probable label to the input x
int CatsEye_predict(CatsEye *this, double *x)
{
	// forward propagation
	CatsEye_forward(this, x);
	// biggest output means most probable label
	double max = this->o3[0];
	int ans = 0;
	for (int i=1; i<this->out; i++) {
		if (this->o3[i] > max) {
			max = this->o3[i];
			ans = i;
		}
	}
	return ans;
}

// save weights to csv file
int CatsEye_save(CatsEye *this, char *filename)
{
	FILE *fp = fopen(filename, "w");
	if (fp==NULL) return -1;

	fprintf(fp, "%d %d %d\n", this->in, this->hid, this->out);

	int i;
	for (i=0; i<(this->in+1)*this->hid-1; i++) {
		fprintf(fp, "%lf ", this->w1[i]);
	}
	fprintf(fp, "%lf\n", this->w1[i]);

	for (i=0; i<(this->hid+1)*this->out-1; i++) {
		fprintf(fp, "%lf ", this->w2[i]);
	}
	fprintf(fp, "%lf\n", this->w2[i]);

	fclose(fp);
	return 0;
}

// save weights to json file
int CatsEye_saveJson(CatsEye *this, char *filename)
{
	FILE *fp = fopen(filename, "w");
	if (fp==NULL) return -1;

	fprintf(fp, "var config = [%d,%d,%d];\n", this->in, this->hid, this->out);

	int i;
	fprintf(fp, "var w1 = [");
	for (i=0; i<(this->in+1)*this->hid-1; i++) {
		fprintf(fp, "%lf,", this->w1[i]);
	}
	fprintf(fp, "%lf];\n", this->w1[i]);

	fprintf(fp, "var w2 = [");
	for (i=0; i<(this->hid+1)*this->out-1; i++) {
		fprintf(fp, "%lf,", this->w2[i]);
	}
	fprintf(fp, "%lf];\n", this->w2[i]);

	fclose(fp);
	return 0;
}

// save weights to binary file
int CatsEye_saveBin(CatsEye *this, char *filename)
{
	FILE *fp = fopen(filename, "wb");
	if (fp==NULL) return -1;

	fwrite(&this->in, sizeof(this->in), 1, fp);
	fwrite(&this->hid, sizeof(this->hid), 1, fp);
	fwrite(&this->out, sizeof(this->out), 1, fp);

	//fwrite(this->w1, sizeof(double)*(this->in+1)*this->hid, 1, fp);
	//fwrite(this->w2, sizeof(double)*(this->hid+1)*this->out, 1, fp);
	for (int i=0; i<(this->in+1)*this->hid; i++) {
		float a = this->w1[i];
		fwrite(&a, sizeof(float), 1, fp);
	}
	for (int i=0; i<(this->hid+1)*this->out; i++) {
		float a = this->w2[i];
		fwrite(&a, sizeof(float), 1, fp);
	}

	fclose(fp);
	return 0;
}
