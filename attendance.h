#ifndef ATTENDANCE_H
#define ATTENDANCE_H

#define MAX_COURSES 20
#define MAX_STUDENTS 200
#define MAX_DAYS 200

typedef struct {
    char course_code[16];
    int days; // number of recorded days
    int matrix[MAX_STUDENTS][MAX_DAYS]; // 1 = present, 0 = absent
    int student_count;
} CourseAttendance;

void init_course_attendance(CourseAttendance *c, const char *code, int students);
int record_attendance_day(CourseAttendance *c, int day_index, int present_array[]);
double compute_attendance_pct(CourseAttendance *c, int student_index);
int compute_absences(CourseAttendance *c, int student_index);
int load_attendance_csv(const char *filename, CourseAttendance courses[], int *course_count);
int save_attendance_csv(const char *filename, CourseAttendance courses[], int course_count);

#endif
