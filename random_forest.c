#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "random_forest.h"



static double g_rand_double(unsigned *seed) {
    *seed = (*seed * 1103515245u + 12345u) & 0x7fffffff;
    return (double)(*seed) / 2147483647.0;
}

void rf_init(RandomForest *rf, int tree_count, int max_depth, int seed){
    rf->tcount = tree_count;
    rf->seed = seed ? seed : 1;
    rf->max_depth = max_depth;
    rf->trees = malloc(sizeof(SimpleTree) * tree_count);
    memset(rf->trees, 0, sizeof(SimpleTree) * tree_count);
}

void rf_free(RandomForest *rf){
    if (rf->trees) free(rf->trees);
    rf->trees = NULL;
    rf->tcount = 0;
}

//compute Gini for binary classification split
static double gini_split(int left_count, int left_positive, int right_count, int right_positive) {
    double lprob = left_count ? (double)left_positive / left_count : 0.0;
    double rprob = right_count ? (double)right_positive / right_count : 0.0;
    double gleft = 1.0 - (lprob*lprob + (1-lprob)*(1-lprob));
    double gright = 1.0 - (rprob*rprob + (1-rprob)*(1-rprob));
    double total = left_count + right_count;
    return (left_count * gleft + right_count * gright) / (double)total;
}

// mse
static double mse_split(int left_count, double left_sum, double left_sqsum, int right_count, double right_sum, double right_sqsum) {
    double left_mse = 0.0, right_mse = 0.0;
    if (left_count) {
        double left_mean = left_sum / left_count;
        left_mse = left_sqsum / left_count - left_mean * left_mean;
    }
    if (right_count) {
        double right_mean = right_sum / right_count;
        right_mse = right_sqsum / right_count - right_mean * right_mean;
    }
    double total = left_count + right_count;
    return (left_count * left_mse + right_count * right_mse) / total;
}

// train one single tree using bootstrap samples 
static void train_tree_classification(SimpleTree *tree, double X[][32], int n, int features, int y[], int max_depth, unsigned *seed) {

    tree->node_count = 0;
    // bootstrap 
    int *inbag = malloc(sizeof(int) * n);
    for (int i=0;i<n;i++) inbag[i]=0;
    int bsamples = n;
    for (int i=0;i<bsamples;i++){
        int idx = (int)(g_rand_double(seed) * n);
        if (idx < 0) idx = 0;
        if (idx >= n) idx = n-1;
        inbag[idx] = 1;
    }

    // brute force search over features
    double best_score = 1e9;
    int best_f = 0;
    double best_thresh = 0;
    for (int f=0; f<features; f++){
        // collect values
        double vals[512];
        int m = 0;
        for (int i=0;i<n;i++){
            vals[m++] = X[i][f];
        }
        // try thresholds 
        for (int t=0;t<m;t++){
            double thresh = vals[t];
            int left_c = 0, left_pos = 0, right_c = 0, right_pos = 0;
            for (int i=0;i<n;i++){
                if (!inbag[i]) continue;
                if (X[i][f] <= thresh) {
                    left_c++;
                    left_pos += (y[i] == 1);
                } else {
                    right_c++;
                    right_pos += (y[i] == 1);
                }
            }
            if (left_c == 0 || right_c == 0) continue;
            double g = gini_split(left_c, left_pos, right_c, right_pos);
            if (g < best_score) {
                best_score = g;
                best_f = f;
                best_thresh = thresh;
            }
        }
    }

    // root node
    TreeNodeSimple root;
    root.feature_index = best_f;
    root.threshold = best_thresh;
    root.is_leaf = 0;
    // compute leaf values
    int left_count = 0, left_pos = 0, right_count = 0, right_pos = 0;
    for (int i=0;i<n;i++){
        if (!inbag[i]) continue;
        if (X[i][best_f] <= best_thresh) {
            left_count++; left_pos += (y[i]==1);
        } else {
            right_count++; right_pos += (y[i]==1);
        }
    }
    root.left_value = left_count ? (double)left_pos / left_count : 0.0;
    root.right_value = right_count ? (double)right_pos / right_count : 0.0;
    root.leaf_value = -1;
    tree->nodes[0] = root;
    tree->node_count = 1;

    free(inbag);
}

// reg tree
static void train_tree_regression(SimpleTree *tree, double X[][32], int n, int features, double y[], int max_depth, unsigned *seed) {
    tree->node_count = 0;
    int *inbag = malloc(sizeof(int) * n);
    for (int i=0;i<n;i++) inbag[i]=0;
    int bsamples = n;
    for (int i=0;i<bsamples;i++){
        int idx = (int)(g_rand_double(seed) * n);
        if (idx < 0) idx = 0;
        if (idx >= n) idx = n-1;
        inbag[idx] = 1;
    }

    double best_score = 1e18;
    int best_f = 0;
    double best_thresh = 0;
    for (int f=0; f<features; f++){
        double vals[512];
        int m=0;
        for (int i=0;i<n;i++) vals[m++] = X[i][f];
        for (int t=0;t<m;t++){
            double thresh = vals[t];
            int lcount=0, rcount=0;
            double lsum=0, lss=0, rsum=0, rss=0;
            for (int i=0;i<n;i++){
                if (!inbag[i]) continue;
                if (X[i][f] <= thresh) {
                    lcount++; lsum += y[i]; lss += y[i]*y[i];
                } else {
                    rcount++; rsum += y[i]; rss += y[i]*y[i];
                }
            }
            if (lcount==0 || rcount==0) continue;
            double mse = mse_split(lcount, lsum, lss, rcount, rsum, rss);
            if (mse < best_score){
                best_score = mse; best_f = f; best_thresh = thresh;
            }
        }
    }

    TreeNodeSimple root;
    root.feature_index = best_f;
    root.threshold = best_thresh;
    root.is_leaf = 0;
    int lcount=0; double lsum=0; int rcount=0; double rsum=0;
    for (int i=0;i<n;i++){
        if (!inbag[i]) continue;
        if (X[i][best_f] <= best_thresh) { lcount++; lsum+=y[i]; }
        else { rcount++; rsum+=y[i]; }
    }
    root.left_value = lcount ? lsum / lcount : 0.0;
    root.right_value = rcount ? rsum / rcount : 0.0;
    root.leaf_value = -1;
    tree->nodes[0] = root;
    tree->node_count = 1;

    free(inbag);
}

void rf_train_classification(RandomForest *rf, double X[][32], int n, int features, int y[]){
    unsigned seed = rf->seed;
    for (int t=0;t<rf->tcount;t++){
        train_tree_classification(&rf->trees[t], X, n, features, y, rf->max_depth, &seed);
    }
    rf->seed = seed;
}

int rf_predict_classification(RandomForest *rf, double x[], int features){
    int votes = 0;
    for (int t=0;t<rf->tcount;t++){
        SimpleTree *tr = &rf->trees[t];
        TreeNodeSimple root = tr->nodes[0];
        double prob = (x[root.feature_index] <= root.threshold) ? root.left_value : root.right_value;
        votes += (prob >= 0.5) ? 1 : 0;
    }
    // majority vote
    return (votes * 2 >= rf->tcount) ? 1 : 0;
}

void rf_train_regression(RandomForest *rf, double X[][32], int n, int features, double y[]){
    unsigned seed = rf->seed;
    for (int t=0;t<rf->tcount;t++){
        train_tree_regression(&rf->trees[t], X, n, features, y, rf->max_depth, &seed);
    }
    rf->seed = seed;
}

double rf_predict_regression(RandomForest *rf, double x[], int features){
    double sum = 0.0;
    for (int t=0;t<rf->tcount;t++){
        SimpleTree *tr = &rf->trees[t];
        TreeNodeSimple root = tr->nodes[0];
        double val = (x[root.feature_index] <= root.threshold) ? root.left_value : root.right_value;
        sum += val;
    }
    return sum / rf->tcount;
}

// OOB
double rf_oob_classification_error(RandomForest *rf, double X[][32], int n, int features, int y[]){
    int mistakes = 0;
    for (int i=0;i<n;i++){
        int votes = 0, vcount=0;
        for (int t=0;t<rf->tcount;t++){
            if ((i + t) % 2 == 0) continue;
            double prob = rf->trees[t].nodes[0].feature_index >= 0 ? ((X[i][rf->trees[t].nodes[0].feature_index] <= rf->trees[t].nodes[0].threshold) ? rf->trees[t].nodes[0].left_value : rf->trees[t].nodes[0].right_value) : 0.0;
            votes += (prob >= 0.5) ? 1 : 0;
            vcount++;
        }
        if (vcount==0) continue;
        int pred = (votes * 2 >= vcount) ? 1 : 0;
        if (pred != y[i]) mistakes++;
    }
    return (double)mistakes / n;
}

double rf_oob_regression_rmse(RandomForest *rf, double X[][32], int n, int features, double y[]){
    double sse = 0.0;
    int count = 0;
    for (int i=0;i<n;i++){
        double sum = 0.0; int vcount = 0;
        for (int t=0;t<rf->tcount;t++){
            if ((i + t) % 2 == 0) continue;
            double val = (X[i][rf->trees[t].nodes[0].feature_index] <= rf->trees[t].nodes[0].threshold) ? rf->trees[t].nodes[0].left_value : rf->trees[t].nodes[0].right_value;
            sum += val; vcount++;
        }
        if (vcount==0) continue;
        double pred = sum / vcount;
        double diff = pred - y[i];
        sse += diff * diff;
        count++;
    }
    if (count==0) return 0.0;
    return sqrt(sse / count);
}
