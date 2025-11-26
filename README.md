# GUI Performance Tracker and Predictor

A comprehensive desktop application built in C with OpenGL that tracks student attendance and academic performance while predicting future outcomes using machine learning algorithms.

## 📋 Project Overview

This project implements a dual-function system that:
- **Tracks attendance** in real-time and calculates absence statistics for each course
- **Monitors academic performance** by tracking grades and displaying them alongside attendance data
- **Predicts future outcomes** using machine learning models to forecast:
  - Probability of achieving 80% attendance
  - Expected end-of-semester GPA

## 🎯 Problem Statement

Students often struggle to predict their academic performance and attendance patterns throughout the semester. This application provides data-driven insights to help students make informed decisions about their academic journey by analyzing existing data to predict end-of-semester outcomes.

## 🛠️ Technical Implementation

### Core Technologies
- **Vanilla C** (no external libraries)
- **OpenGL** for GUI implementation
- **Custom ML algorithms** implemented from scratch

### Machine Learning Models
- **Random Forest Algorithm** (primary model)
  - Bootstrap sampling for dataset subsets
  - Random feature selection at each split
  - Majority voting for classification tasks
  - Mean averaging for regression task
