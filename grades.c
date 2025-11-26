#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "grades.h"
#include "utils.h"

void init_course_grades(CourseGrades *g, const char *code, int credits){
    strncpy(g->course_code, code, 15);
    g->course_code[15] = 0;
    g->component_count = 0;
    g->credits = credits;
    for (int i=0;i<MAX_COMPONENTS;i++)
        for (int j=0;j<MAX_STUDENTS;j++)
            g->scores[j][i] = 0.0;
    for (int i=0;i<MAX_COMPONENTS;i++)
        g->weights[i] = 0.0;
}

double compute_course_percentage(CourseGrades *g){
    double sum = 0.0, wsum = 0.0;
    for(int i=0;i<g->component_count;i++){
        for(int s=0;s<MAX_STUDENTS;s++)
            sum += g->scores[s][i]*g->weights[i];
        wsum += g->weights[i]*MAX_STUDENTS;
    }
    return wsum>0 ? sum/wsum : 0.0;
}

double compute_course_percentage_student(CourseGrades *g, int student_idx){
    double sum = 0.0, wsum = 0.0;
    for (int i=0;i<g->component_count;i++){
        sum += g->scores[student_idx][i]*g->weights[i];
        wsum += g->weights[i];
    }
    return wsum>0 ? sum/wsum : 0.0;
}

double percentage_to_gpa(double pct){
    if (pct >= 90) return 4.0;
    if (pct >= 85) return 3.7;
    if (pct >= 80) return 3.3;
    if (pct >= 75) return 3.0;
    if (pct >= 70) return 2.7;
    if (pct >= 65) return 2.3;
    if (pct >= 60) return 2.0;
    if (pct >= 50) return 1.0;
    return 0.0;
}

double compute_cgpa(CourseGrades courses[], int course_count){
    double num = 0.0;
    int den = 0;
    for (int i=0;i<course_count;i++){
        double pct = compute_course_percentage(&courses[i]);
        double gpa = percentage_to_gpa(pct);
        num += gpa * courses[i].credits;
        den += courses[i].credits;
    }
    return den>0 ? num/den : 0.0;
}

double compute_cgpa_student(CourseGrades courses[], int course_count, int student_idx){
    double num = 0.0;
    int den = 0;
    for(int i=0;i<course_count;i++){
        double pct = compute_course_percentage_student(&courses[i], student_idx);
        double gpa = percentage_to_gpa(pct);
        num += gpa * courses[i].credits;
        den += courses[i].credits;
    }
    return den>0 ? num/den : 0.0;
}

int load_grades_csv(const char *filename, CourseGrades courses[], int *course_count){
    FILE *f = fopen(filename,"r");
    if(!f) return 0;
    char line[2048];
    int idx = 0;
    while(fgets(line,sizeof(line),f)){
        if(line[0]=='#' || strlen(line)<2) continue;
        char code[32]; int credits, comp;
        if(sscanf(line,"%31[^,],%d,%d",code,&credits,&comp)!=3) continue;
        init_course_grades(&courses[idx], code, credits);
        courses[idx].component_count = comp;
        // weights
        if(!fgets(line,sizeof(line),f)) break;
        trim_newline(line);
        char *tok = strtok(line,",");
        int j=0;
        while(tok && j<comp){
            courses[idx].weights[j]=atof(tok);
            tok=strtok(NULL,",");
            j++;
        }
        // scores
        if(!fgets(line,sizeof(line),f)) break;
        trim_newline(line);
        tok=strtok(line,",");
        j=0;
        int s=0;
        while(tok && j<comp && s<MAX_STUDENTS){
            courses[idx].scores[s][j]=atof(tok);
            tok=strtok(NULL,",");
            j++;
            if(j>=comp){ j=0; s++; }
        }
        idx++;
        if(idx>=MAX_COURSES) break;
    }
    fclose(f);
    *course_count = idx;
    return 1;
}

int save_grades_csv(const char *filename, CourseGrades courses[], int course_count){
    FILE *f = fopen(filename,"w");
    if(!f) return 0;
    for(int i=0;i<course_count;i++){
        fprintf(f,"%s,%d,%d\n",courses[i].course_code,courses[i].credits,courses[i].component_count);
        for(int j=0;j<courses[i].component_count;j++){
            fprintf(f,"%g",courses[i].weights[j]);
            if(j+1<courses[i].component_count) fprintf(f,",");
        }
        fprintf(f,"\n");
        for(int s=0;s<MAX_STUDENTS;s++){
            for(int j=0;j<courses[i].component_count;j++){
                fprintf(f,"%g",courses[i].scores[s][j]);
                if(j+1<courses[i].component_count) fprintf(f,",");
            }
            fprintf(f,"\n");
        }
    }
    fclose(f);
    return 1;
}
