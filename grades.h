#ifndef GRADES_H
#define GRADES_H

#define MAX_COMPONENTS 20

typedef struct {
    char course_code[16];
    int component_count;
    double weights[MAX_COMPONENTS]; // weights sum to 1.0 roughly
    double scores[MAX_COMPONENTS]; // current obtained scores (percentage 0-100)
    int credits; // credit hours for GPA conversion
} CourseGrades;

void init_course_grades(CourseGrades *g, const char *code, int credits);
double compute_course_percentage(CourseGrades *g);
double percentage_to_gpa(double pct);
double compute_cgpa(CourseGrades courses[], int course_count);

int load_grades_csv(const char *filename, CourseGrades courses[], int *course_count);
int save_grades_csv(const char *filename, CourseGrades courses[], int course_count);

#endif
