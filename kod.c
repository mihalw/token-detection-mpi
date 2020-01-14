#include <mpi.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define MSG_TOKEN 100
#define MSG_CONF 5
#define MSG_SIZE 1
#define SUCCESS_THRESHOLD 4
#define PROBABILITY_CONF 2
#define TIME_IN_CRITICAL_SECTION 10

void my_send(int my_rank, int what_to_send, int receiver, int msg_name, MPI_Request req, int *last_sended_color, int color_received)
{
	if (msg_name == MSG_TOKEN)
	{
		printf("%d: Wyslalem token w kolorze %d do procesu %d\n", my_rank, what_to_send, receiver);
		if ((rand() % 10) > SUCCESS_THRESHOLD)
		{
			MPI_Isend(&what_to_send, MSG_SIZE, MPI_INT, receiver, msg_name, MPI_COMM_WORLD, &req);
			*last_sended_color = what_to_send;
		}
	}
	else if (msg_name == MSG_CONF)
	{
		printf("%d: Wyslalem potwierdzenie otrzymania tokenu w kolorze %d do procesu %d\n", my_rank, color_received, receiver);
		if ((rand() % 10) > SUCCESS_THRESHOLD)
		{
			MPI_Isend(&what_to_send, MSG_SIZE, MPI_INT, receiver, msg_name, MPI_COMM_WORLD, &req);
		}
	}
}

int main(int argc, char **argv)
{
	srand(time(0));
	bool token_wait_started = false, conf_wait_started = false, can_send_token = false;
	int color_received, last_sended_color = 1, last_received_color = 1, color_to_send = -1; // -1 to bialy, 1 to czarny
	int my_rank, size, confirmation;
	int flag_received = 0, flag_conf_recv = 0;
	MPI_Status status_received, status_send, status_conf_recv, status_conf_send;
	MPI_Request req_send, req_conf, req_rcv, req_ack;
	time_t receive_token_time, leave_critical_section_time = 0;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	if (my_rank == 0)
	{
		can_send_token = true;
	}

	while (1)
	{
		sleep(rand() % 3 + 1);
		if (can_send_token && time(NULL) > leave_critical_section_time)
		{
			my_send(my_rank, color_to_send, (my_rank + 1) % size, MSG_TOKEN, req_send, &last_sended_color, 0);

			if (!conf_wait_started)
			{
				MPI_Irecv(&confirmation, MSG_SIZE, MPI_INT, (my_rank + 1) % size, MSG_CONF, MPI_COMM_WORLD, &req_conf);
				conf_wait_started = true;
			}

			MPI_Test(&req_conf, &flag_conf_recv, &status_conf_send);

			if (flag_conf_recv)
			{
				printf("%d: Potwierdzenie odebrania tokenu przez proces %d, przestaje go juz wysylac\n", my_rank, status_conf_send.MPI_SOURCE);

				can_send_token = false;
				flag_conf_recv = false;
				conf_wait_started = false;
			}
		}

		if (!token_wait_started)
		{
			MPI_Irecv(&color_received, MSG_SIZE, MPI_INT, (my_rank + (size - 1)) % size, MSG_TOKEN, MPI_COMM_WORLD, &req_rcv);
			token_wait_started = true;
		}

		MPI_Test(&req_rcv, &flag_received, &status_received);

		if (flag_received)
		{
			token_wait_started = false;

			if (color_received != last_received_color)
			{
				last_received_color = color_received;
				color_to_send = -last_sended_color;

				printf("%d: Otrzymalem token w kolorze %d od procesu %d\n", my_rank, color_received, status_received.MPI_SOURCE);
				can_send_token = true;

				my_send(my_rank, confirmation, (my_rank + (size - 1)) % size, MSG_CONF, req_ack, 0, color_received);
				receive_token_time = time(NULL);
				leave_critical_section_time = receive_token_time + TIME_IN_CRITICAL_SECTION;
			}
			flag_received = false;
		}
	}
	MPI_Finalize();
}
