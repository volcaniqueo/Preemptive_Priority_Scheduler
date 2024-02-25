# Preemptive_Priority_Scheduler
This project was my 2nd Homework for the course CMPE 322 (Operating Systems) at Bogazici University.

## About the Project
This project is an implementation for the preemptive priority scheduling mechanism which can typically be used in operating system schedulers. The exact rules for the scheduling mechanims can be found in the project description. The format of the '.txt' files in this repository can also be found in the project description file. There also exists example input & output folder to test the implementation. The implementation approach can be found in the project report.

## To Run the Code
First make sure the Makefile, definition.txt and the scheduler.c is in the same directory. Then, to create the executable run the Makefile with:

```make```

Now, you can run the executable with an example input with:

```./scheduler```

Please refer to the project description file for the format of the output file.

## Final Remarks
Since so many parameters for the scheduling was fixed (e.g. the processes are always the same), the implementation was pretty starighforward. A simple array iteration is used to implement the project and it was pretty enough for the scope of this project. Of course, a normal scheduler has no any priori fixed information; thus, normal scheduler would have been required a priorty queue machanism and an appropriate compare function, i.e., a DES mechanism (Discrete Event Simulation).
