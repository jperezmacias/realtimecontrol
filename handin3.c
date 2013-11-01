#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/io.h>
#include <sys/mman.h>
#include <native/task.h>
#include <native/timer.h>
#include <native/mutex.h>

#define STACK_SIZE 8192
#define STD_PRIO 1

RT_TASK process;
RT_TASK controller;
RT_TASK reference;

RT_MUTEX signals_mutex;

int int_count = 0;
int end = 0;

double r = 0.0;
double y = 0.0;
double u = 0.0;
double r_buf[60];
double y_buf[60];
double u_buf[60];

void f_process(void *cookie)
{
	int ret;
	unsigned long overruns;
	int count = 0;

	// sampling time in ns
	RTIME h_ns = 35000000u;

	// old values
	double u1 = 0;
	double u2 = 0;
	double y1 = 0; //y(k-1)
	double y2 = 0; //y(k-2)

	// systems constants
	double a2 = 0.5984; //u(k-2)
	double a1 = 0.6054; //u(k-1)

	double b2 = 0.9656; //y(k-2)
	double b1 = -1.966; //y(k-1)

	ret = rt_task_set_periodic(NULL, TM_NOW, rt_timer_ns2ticks(h_ns));
	if (ret) {
		printf("error while set periodic, code %d\n", ret);
		return;
	}

	RT_TASK_INFO info;
	rt_task_inquire(rt_task_self(), &info);
	printf("Starting task %s \n", info.name);
	fflush(NULL);

	while(!end)
	{
		rt_mutex_acquire(&signals_mutex, rt_timer_ns2ticks(10000));
		y = a1*u1 + a2*u2 - b1*y1 - b2*y2;
		
		r_buf[int_count] = r;
		y_buf[int_count] = y;
		u_buf[int_count] = u;
		if (int_count==59)
		{
			end=1;
		}
		int_count++;

		// update state
		y2 = y1;
		y1 = y;
		u2 = u1;
		u1 = u;

		rt_mutex_release(&signals_mutex);
		printf("t = \t %d;\t", count);	
		printf("y = \t %f\n", y1);	

		count++;

		ret = rt_task_wait_period(&overruns);
		if (ret) {
			printf("error while process rt_task_wait_period, code %d\n",ret);
			return;
		}
	}
}

void f_controller(void *cookie)
{
	int ret;
	unsigned long overruns;

	// sampling time in ns
	RTIME h_ns = 35000000u;
	double e;
	// old values
	double e1 = 0;
	double u1 = 0;

	// systems constants
	double a1 = -0.2184; //e(k-1)
	double a0 = 0.2364; //e(k)

	double b1 = -0.2157;//-0.2157; //u(k-1)

	ret = rt_task_set_periodic(NULL, TM_NOW, rt_timer_ns2ticks(h_ns));
	if (ret) {
		printf("error while set periodic, code %d\n", ret);
		return;
	}

	RT_TASK_INFO info;
	rt_task_inquire(rt_task_self(), &info);
	printf("Starting task %s \n", info.name);
	fflush(NULL);

	while(!end)
	{
		rt_mutex_acquire(&signals_mutex, rt_timer_ns2ticks(10000));
		e = r-y;
		u = a0*e + a1*e1 - b1*u1;

		// update state
		u1 = u;
		e1 = e;

		printf("e: %'.2f\tu: %'.2f\n", e,u);

		rt_mutex_release(&signals_mutex);

		ret = rt_task_wait_period(&overruns);
		if (ret) {
			printf("error while controller rt_task_wait_period, code %d\n",ret);
			return;
		}
	}
}
void f_reference(void *cookie)
{
	int ret;
	unsigned long overruns;

	// sampling time in ns
	RTIME h_ns = 1000000000u;

	ret = rt_task_set_periodic(NULL, TM_NOW, rt_timer_ns2ticks(h_ns));
	if (ret) {
		printf("error while set periodic, code %d\n", ret);
		return;
	}

	RT_TASK_INFO info;
	rt_task_inquire(rt_task_self(), &info);
	printf("Starting task %s \n", info.name);
	fflush(NULL);

	while(!end)
	{
		printf("reference: %'.2f\n", r);

		ret = rt_task_wait_period(&overruns);
		if (ret) {
			printf("error while controller rt_task_wait_period, code %d\n",ret);
			return;
		}
		rt_mutex_acquire(&signals_mutex, rt_timer_ns2ticks(10000));
		r = -r;
		rt_mutex_release(&signals_mutex);

	}
}
// signal-handler, to ensure clean exit on Ctrl-C
void clean_exit(int dummy) 
{
	printf("cleanup\n");
	end = 1;

	// Delete tasks
	rt_task_delete(&process);
	rt_task_delete(&controller);
	rt_task_delete(&reference);

	// delete mutex
	rt_mutex_delete(&signals_mutex);

	printf("end\n");
	fflush(NULL);
	
	FILE *fp;
	int i;
   
	fp = fopen("results.dat", "w");
	if (fp == NULL) {
		printf("I couldn't open results.dat for writing.\n");
		exit(0);
	}
   
	for (i=0; i<60; ++i)
		fprintf(fp, "%f\t%f\t%f\n", r_buf[i], u_buf[i], y_buf[i]);
	/* close the file */
	fclose(fp);
}

int main(int argc, char *argv[]) 
{
	int err, ret;
	printf("start\n");
	// init signals
	r = 1;
	u = 0;
	y = 0;

	// install signal handler
	signal(SIGTERM, clean_exit);	
	signal(SIGINT, clean_exit);	

	mlockall(MCL_CURRENT | MCL_FUTURE);

	// Create process task
	err = rt_task_spawn(&process, "Process", STACK_SIZE, STD_PRIO, 0, &f_process, NULL);
	if (err) 
 	{
		printf("error rt_task_spawn: %d\n", err);
		return 0;
	}

	// Create controller task
	err = rt_task_spawn(&controller, "Controller", STACK_SIZE, STD_PRIO, 0, &f_controller, NULL);
	if (err) 
	{
		printf("error rt_task_spawn: %d\n", err);
		return 0;
	}

	// Create reference task
	err = rt_task_spawn(&reference, "Reference", STACK_SIZE, STD_PRIO, 0, &f_reference, NULL);
	if (err) 
	{
		printf("error rt_task_spawn: %d\n", err);
		return 0;
	}

	// wait for signal & return of signal handler
	pause();
	fflush(NULL);
	return 0;
}
