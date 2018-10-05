# Linux Job Management System (jms)

This project is an implementation of a job management system for linux (and other UNIX-like Operating Systems).
## Overview of the System

The jms consists of two different parts: the jms_coord and the jms_console.
* The jms_coord is responsible for managing jobs (processes). jms_coord is not managing the jobs directly, but with the help of pools, that each one handles a maximum number of jobs. These pools are created dynamicaly as needed for serving all the submited jobs. When they have served their maximum number of jobs, they get removed, after they have reported their stats. The communication between the pools and the jms_coord is done via named pipes.
* The jms_console handles all the input/output from/to the user for submitting and checking the status of various processes. This program communicates with the jms_coord via named pools.

![overview of the system](https://imgur.com/oGeA5RU.jpg)

### About the jobs output
It should be noted that the job's outputs (stdout & stderr) are redirected to two files called

```stdout_jobid```
```stderr_jobid```
where **jobid** is the id of the job

These files will be located on a directory with a specific name:

```sdi1400139_jobid_pid_date_time```
* **jobid** the id of the job
* **pid** the linux pid of the job
* **date** date in the from of YearMonthDate e.g. 20181005
* **time** time in the form of Hours:Minutes:Seconds e.g. 17:50:02

Also a bash script is provided for better managing these dirs/files called ```jms_script.sh```

## Getting Started

The following steps are required in order to set up your enviroment and run the above programs.

### Prerequisites

Apart from having a linux system with a gcc compiler there are no other requirements.

### Installing

1) Get a copy of the project
```
git clone https://github.com/v-pap/linux-job-management-system.git
```
2) Go into the directory
```
cd linux-job-management-system
```
3) Compile the project
```
make
```

## Using the program(s)

In order to run the program(s) you will have to use one one of the following commands:

1) **jms_coord**
```
./jms_coord -l <path> -n <jobs_pool> -w <jms_in> -r <jms_out>
```
* **path** name of the directory where all the directories and files created during the usage of the jms_coord will be saved to
* **jobs_pool** maximum number of jobs per pool
* **jms_in** named pipe for reading data coming from the jms_console
* **jms_out** named pipe for writing data to the jms_console

2) **jms_console**
```
./jms_console -w <jms_in> -r <jms_out> -o <operations_file>
```
* **jms_in** named pipe for reading data coming from the jms_coord
* **jms_out** named pipe for writing data to the jms_coord
* **operations_file** optional file which contains user commands (instead of using the stdin) 

3) **jms_script.sh**
```
./jms_script.sh -l <path> -c <command>
```
* **path** name of the directory where all the directories and files created during the usage of the jms_coord were saved
* **command** the name of the command to be used by the script (one of those below)
1) **list** shows a list with all the directories that the jobs created
2) **size <n>** shows a list with all the directories that the jobs created, sorted by their size.
  Optionally the n can be specify the first n entries to be shown.
3) **purge** deletes all the directories that the jobs created

### List of available commands
The following commands can be run on jms_console after jms_coord has been initialized
1) ```submit <job>```

* Submits a job to the jms

* **job** a process to be run e.g. sleep 30


2) ```status <JobID>```

* Returns the status of the job

* **JobID** the id of the job

3) ```status-all [time-duration]```

* Returns the status of all the jobs

* **time-duration** optional argument for returning the status of jobs that are running for equal or less time than *time_duration*

4) ```show-active```

* Returns the jobs that are still active

5) ```show-pools```

* Returns the number of jobs for each pool

6) ```show-finished```

* Returns the jobs that have finished

7) ```suspend <JobID>```

* Suspends a job

* **JobID** the id of the job

8) ```resume <JobID>```

* Resumes a job

* **JobID** the id of the job

9) ```shutdown```

* Shuts down the whole system including the managed jobs


## License

This project is licensed under the MIT License
