/*
Student Name: Sait Talha Nisanci
Student Number: 2013400186
Compile Status: Compiling
Program Status: Running
*/
#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"

FILE *cout;
int threshold = 25; // Can be 10,25,40
int counter =0; // A variable for indexing
int i = 0; int j = 0;
int** arr;
int* original_arr; // 1D array for input.txt
int main(int argc, char* argv[]){
				int rank; // rank of the current processor
				int size; // total number of processors
				MPI_Init(&argc, &argv);
				MPI_Comm_rank(MPI_COMM_WORLD, &rank); // gets the rank of the current processor
				MPI_Comm_size(MPI_COMM_WORLD, &size); // gets the total number of processors

				int row_per_processor = 200/(size-1); // How many rows each of processors will have
				if(rank==0){

								threshold=atoi(argv[3]);
								FILE *cin = fopen(argv[1], "r"); // reads from argument
								cout = fopen(argv[2],"w"); // reads from argument
								int *arr[200];  //input array in 2D
								for( ; i<200;i++){
												arr[i] = (int * ) malloc(200*sizeof(int));
								}
								original_arr = (int *) malloc((200+row_per_processor)*200*sizeof(int));

								i = 0;
								counter = row_per_processor*200; //In order to skip master processor we give this value to counter so that master won't have any piece of input data
								for(i=0; i<200;i++) {
												for(j=0; j<200;j++) {

																fscanf(cin,"%d",&arr[i][j]); //get input
																original_arr[counter] = arr[i][j]; 
																counter++; // increase index

												}
								}
				}
								MPI_Bcast(&threshold,1,MPI_INT,0,MPI_COMM_WORLD);
				// Wait until master processor has completed getting input.
				MPI_Barrier(MPI_COMM_WORLD);
				int *sub_arr[row_per_processor+2];//Holds each processors part as 2D  
				int *local = (int *) malloc(200*(row_per_processor)*sizeof(int));// array to get 2d array as 1d from master processor.

				i = 0; j = 0;
				for( ; i<row_per_processor+2;i++){
								sub_arr[i] = (int * ) malloc(200*sizeof(int));//allocate memory	
				}
				//Wait until all processors allocated memory.
				MPI_Barrier(MPI_COMM_WORLD);
				// scatter the original array to each proessor
				MPI_Scatter(original_arr,row_per_processor*200,MPI_INT,local,row_per_processor*200,MPI_INT,0,MPI_COMM_WORLD);
				counter = 0; 
				// convert 1d local array to 2d sub array
				for(i = 1; i< row_per_processor+1 ;i++){
								for(j =0; j<200;j++){
												sub_arr[i][j] = local[counter];//Fill 2D array from the 1D local array
												counter++;//increment index
								}
				}
				int* smoothed_arr; //Holds smoothed values in 1D array
				int* received_arr = (int *) malloc(200*sizeof(int));//Will hold the last row of (rank-1) th processor
				int* received_arr2 = (int *) malloc(200*sizeof(int));//Wild hold the first row of (rank+1) th processor
				// fill received_arr and received_arr2 with 0		
				for(j = 0 ; j<200;j++){
								received_arr[j] = 0;
								received_arr2[j] =0;
				}
				if(rank==1 || rank==size-1){
								smoothed_arr= (int *) malloc(198*(row_per_processor+5)*sizeof(int));
				}else {
								smoothed_arr = (int *) malloc(198*(row_per_processor+5)*sizeof(int));
				}
				//wait until all processors have the necessarry arrays for exchanging borders
				MPI_Barrier(MPI_COMM_WORLD);
				//send borders for odd indexed ones to the upper processor
				//send borders for even indexed ones to the lower processor
				if (rank%2==1 && rank !=size-1 ) {
								MPI_Send(sub_arr[row_per_processor], 200, MPI_INT, rank+1, 0, MPI_COMM_WORLD); // Send last row of (rank) th processor to (rank+1) th processor
								MPI_Recv(received_arr2, 200, MPI_INT, rank+1, 0, MPI_COMM_WORLD,MPI_STATUS_IGNORE); // Receive first row of (rank+1) th processor
				}  else if (rank%2==0 && rank!=0) {
								MPI_Recv(received_arr, 200, MPI_INT, rank-1, 0, MPI_COMM_WORLD,MPI_STATUS_IGNORE); //Receive last row of (rank-1) th processor
								MPI_Send(sub_arr[1], 200, MPI_INT, rank-1, 0, MPI_COMM_WORLD);//Send first row of (rank) th processor to (rank+1) th processor
				}
				//wait until first exchange is completed.
				MPI_Barrier(MPI_COMM_WORLD);

				//send borders for odd indexed ones to the lower processor
				//send borders for even indexed ones to the upper processor
				if (rank%2==1 && rank!=1) {
								MPI_Send(sub_arr[1], 200, MPI_INT, rank-1, 0, MPI_COMM_WORLD);
								MPI_Recv(received_arr, 200, MPI_INT, rank-1, 0, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
				}
				else if (rank%2==0 && rank!=0 && rank!=size-1) {
								MPI_Recv(received_arr2, 200, MPI_INT, rank+1, 0, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
								MPI_Send(sub_arr[row_per_processor], 200, MPI_INT, rank+1, 0, MPI_COMM_WORLD);
				}	
				//wait until second exchange is completed.
				MPI_Barrier(MPI_COMM_WORLD);
/*After getting necessarry boundary rows from rank-1 th and rank +1 th processors with a generic code we can calculate smoothed values without the need of communicating with other processers. We do not put any if/else statement here to prevent calculating 2 unnecesary rows for the sake of readibility.
*/
				//Fill the first and last rows of sub_arr with the communicated arrays send by the neighbor processors
				for(j= 0; j<200;j++){
								sub_arr[0][j] = received_arr[j];
								sub_arr[row_per_processor+1][j]= received_arr2[j];
				}
				counter =0;
				for(i =1; i < row_per_processor+1;i++){
								for(j = 0; j<198;j++){
												int value  =0;
												int o1 =0; int o2 =0;
												//Calculate 3*3 Matrix for smoothing. We only concern about the integer part of that. While calculating the 'value' for each entry get the integer part of 1/9 of that unit.
												for(o1 =0; o1<3;o1++){
																for(o2 =0; o2<3;o2++){
																				value+= (sub_arr[i-1+o1][j+o2]);
																}
												}

												smoothed_arr[counter] =(int)( 1.0*value/9);//Store the smoothed value in an array
												counter++;//increment counter
								}

				} 
				int* last_row = (int *) malloc(198*sizeof(int)); //Last row of each processor's part.
				//Fill last_row with 0
				for(j=0;j< 198;j++){
								last_row[j]=0;
				}
				// Fill the last_row array based on the smoothed values.
				for(j =197;j>=0;j--){
								counter--;
								last_row[j]= smoothed_arr[counter];//Store the last row of each processor to send it to (rank+1) th processor
				}
				MPI_Barrier(MPI_COMM_WORLD);
				//send borders for odd indexed ones to the upper processor
				//send borders for even indexed ones to the lower processor
				//This part is similator to the previous part where we sent border rows to neighbor processors. The logic is exactly the same received_arr2 will hold the first row of (rank+1) th processor and received_arr will hold the last row of (rank-1) th processor
				if (rank%2==1 && rank !=size-1 ) {
								MPI_Send(last_row, 198, MPI_INT, rank+1, 0, MPI_COMM_WORLD);
								MPI_Recv(received_arr2, 198, MPI_INT, rank+1, 0, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
				}
				else if (rank%2==0 && rank!=0) {
								MPI_Recv(received_arr, 198, MPI_INT, rank-1, 0, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
								MPI_Send(smoothed_arr, 198, MPI_INT, rank-1, 0, MPI_COMM_WORLD);
				}
				//wait until first exchange is completed.
				MPI_Barrier(MPI_COMM_WORLD);

				//send borders for odd indexed ones to the lower processor
				//send borders for even indexed ones to the upper processor
				if (rank%2==1 && rank!=1) {
								MPI_Send(smoothed_arr, 198, MPI_INT, rank-1, 0, MPI_COMM_WORLD);
								MPI_Recv(received_arr, 198, MPI_INT, rank-1, 0, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
				}
				else if (rank%2==0 && rank!=0 && rank!=size-1) {
								MPI_Recv(received_arr2, 198, MPI_INT, rank+1, 0, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
								MPI_Send(last_row, 198, MPI_INT, rank+1, 0, MPI_COMM_WORLD);
				}	
				//wait until second exchange is completed.
				MPI_Barrier(MPI_COMM_WORLD);


				// sub_arr will hold smoothed values for each processor
				counter = 0;
				for(j = 0 ; j  < 198;j++){
								sub_arr[0][j] = received_arr[j];
				}
				for(i =1; i <= row_per_processor;i++){
								for(j = 0; j< 198;j++){
												sub_arr[i][j]= smoothed_arr[counter];//Store as 2D
												counter++;
								}
				}
				for(j=0;j<198;j++){
								sub_arr[row_per_processor+1][j]= received_arr2[j];
				}
				MPI_Barrier(MPI_COMM_WORLD);
				int* filtered_arr = (int *) malloc(196*row_per_processor*sizeof(int)); // if 0 then no line indication if 255 then line indication
				counter =0;
				//Apply 4 different filters based on the threshold
				for(i =1; i < row_per_processor+1;i++){
								for(j = 0; j<196;j++){
												int o1= 0;int o2 = 0;
												int value = 0;
												int isAbove= 0;
												// horizontal filter
												for(o2 = 0; o2 < 3;o2++){
																value += -1*sub_arr[i-1][j+o2];
																value	+= -1*sub_arr[i+1][j+o2];		
																value += 2*sub_arr[i][j+o2];
												}
												if(value>threshold) isAbove=1;
												value = 0;
												// vertical filter
												for(o2 = 0; o2 < 3;o2++){
																value += -1*sub_arr[i-1+o2][j];
																value += -1*sub_arr[i-1+o2][j+2];
																value += 2*sub_arr[i-1+o2][j+1];
												}

												if(value>threshold) isAbove=1;
												value = 0;
												// y=-x filter
												for(o1 = 0 ; o1<3;o1++){
																for(o2 = 0 ; o2< 3;o2++){
																				if(o1==o2) value+= 2*sub_arr[i-1+o1][j+o2];
																				else value+= -1*sub_arr[i-1+o1][j+o2];

																}
												}
												
												if(value>threshold) isAbove=1;
												value = 0;
												//y=x filter
												for(o1 = 0 ; o1<3;o1++){
																for(o2 = 0 ; o2< 3;o2++){
																				if(o1+o2==2) value+= 2*sub_arr[i-1+o1][j+o2];
																				else value+= -1*sub_arr[i-1+o1][j+o2];

																}
												}
												if(value>threshold) isAbove=1;
												//If value is greater than or equal to threshold then we will set the value of that pixel to 255 otherwise we will put 0 to that pixel.
												filtered_arr[counter] = isAbove ? 255:0;
												counter++;
								}

				}
				//Wait until all processors are done with filtering.
				MPI_Barrier(MPI_COMM_WORLD);
				int* final_arr= NULL;//This array will hold the filtered values of each pixel so this will consist of only 255 and 0's 255.
				if(rank==0){
								final_arr = (int *) malloc(196*size*(row_per_processor+2)*sizeof(int));		
				}
				//Gather the filtered values to the master processor
				MPI_Gather(filtered_arr, counter, MPI_INT, final_arr, counter, MPI_INT, 0,
											MPI_COMM_WORLD);//Get filtered arrays from each processor and store them in final array.
			

				//Wait until all processors send their array to the master processor
				MPI_Barrier(MPI_COMM_WORLD);
				if(rank==0){
								int counter2= 0;
								for(i = counter+2*196; i < counter*(size)-2*196;i++){
												fprintf(cout,"%d ",final_arr[i]);
												counter2++;
												//If counter2 is equal to 196 then we should put a new line since we will always be given 200*200 input and since after smoothing and filtering we will be left with 196*196 matrix.
												if(counter2==196) {
																counter2 =0;
																fprintf(cout,"\n");
												}
								}	

				}
				MPI_Barrier(MPI_COMM_WORLD);
				MPI_Finalize();


				return 0;
}
