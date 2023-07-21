# Real-time Control System README

This README provides an overview and explanation of the code for a real-time control system implemented in C. The system consists of three real-time tasks: `Process`, `Controller`, and `Reference`. The tasks are managed by the Xenomai Real-Time Operating System (RTOS) library.

![Results](/img/results.png "Results")

## Requirements
To compile and run the code, the following libraries are required:

- `stdio.h`
- `unistd.h`
- `stdlib.h`
- `string.h`
- `signal.h`
- `sys/time.h`
- `sys/io.h`
- `sys/mman.h`
- `native/task.h`
- `native/timer.h`
- `native/mutex.h`

## System Overview
The real-time control system is designed to control a process with feedback using a controller. The controller attempts to drive the process output `y` to follow a reference value `r`.

## Tasks

### 1. Process Task (`f_process`)

This task simulates a physical process and calculates the process output `y` based on the control input `u`. The process model is represented by the following difference equation:

y(k) = a1*u(k-1) + a2*u(k-2) - b1*y(k-1) - b2*y(k-2)


The task samples data at a fixed interval specified by `h_ns`. It also records the process output `y`, control input `u`, and reference value `r` in arrays `y_buf`, `u_buf`, and `r_buf`, respectively, for later analysis. The task runs until `int_count` reaches 60.

### 2. Controller Task (`f_controller`)

This task implements a controller that adjusts the control input `u` based on the error `e`, which is the difference between the reference `r` and the process output `y`. The controller computes the control input `u` using the following difference equation:


u(k) = a0e(k) + a1e(k-1) - b1*u(k-1)


The task samples data at a fixed interval specified by `h_ns`.

### 3. Reference Task (`f_reference`)

This task generates the reference value `r`, which is periodically inverted every second (`h_ns = 1000000000ns`). The task also samples data at a fixed interval.

## Mutex

A mutex (`signals_mutex`) is used to ensure proper synchronization between the tasks when accessing shared variables (`r`, `y`, `u`, etc.).

## Signal Handling

The code includes a signal handler `clean_exit` that allows for a clean exit of the real-time tasks upon receiving `SIGINT` (Ctrl-C) or `SIGTERM`. The signal handler deletes the tasks and the mutex and writes the collected data from the `r_buf`, `y_buf`, and `u_buf` arrays to a file named `results.dat`.

## Building and Running
Ensure that the required libraries and the Xenomai RTOS are properly installed and configured on your system. Then, compile and execute the code using the provided Makefile or your preferred build system.

**Note**: This system is intended for use in a real-time environment, and it requires specific hardware and software setup to ensure real-time behavior.

For more information on Xenomai and its setup, please refer to the official documentation.

## Data Analysis

After running the system, you can analyze the collected data stored in `results.dat`. The file contains three columns: `r`, `u`, and `y`, representing the reference, control input, and process output values, respectively, at each time step.

---
Please make sure to adjust the instructions and explanations in this README as needed, and provide any additional information necessary for users to successfully understand and use the real-time control system.
