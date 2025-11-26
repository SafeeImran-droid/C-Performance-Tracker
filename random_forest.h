#ifndef RANDOM_FOREST_H
#define RANDOM_FOREST_H

#include "attendance.h"
#include "grades.h"

typedef struct {
    int feature_index;
    double threshold;
    double left_value; // prediction left
    double right_value; // prediction right
    int is_leaf;
    double leaf_value;
} TreeNodeSimple;

typedef struct {
    TreeNodeSimple nodes[64];
    int node_count;
} SimpleTree;

typedef struct {
    SimpleTree *trees;
    int tcount;
    int seed;
    int max_depth;
} RandomForest;

void rf_init(RandomForest *rf, int tree_count, int max_depth, int seed);
void rf_free(RandomForest *rf);

void rf_train_classification(RandomForest *rf, double X[][32], int n, int features, int y[]);
int rf_predict_classification(RandomForest *rf, double x[], int features);
void rf_train_regression(RandomForest *rf, double X[][32], int n, int features, double y[]);
double rf_predict_regression(RandomForest *rf, double x[], int features);

// oob estimate 
double rf_oob_classification_error(RandomForest *rf, double X[][32], int n, int features, int y[]);
double rf_oob_regression_rmse(RandomForest *rf, double X[][32], int n, int features, double y[]);

#endif
