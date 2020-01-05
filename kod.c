#include <mpi.h>
#include <stdbool.h>
#include <stdio.h>
#define MSG_HELLO 100
#define MSG_SIZE 1

bool resend = false, can_send_token = true;
int color = -1; // -1 to bialy, 1 to czarny
int last_received_token = -1, last_sended_token = -1;
int rank, sender, size;
int msg[MSG_SIZE];

void SendToken(int sender, int receiver, int color[])
{
	int c[1];
	if (can_send_token)
	{
		can_send_token = false;
		resend = true;
		last_sended_token *= -1;
		c[0] = last_sended_token;
		while (resend)
		{
			MPI_Send(c, MSG_SIZE, MPI_INT, receiver, MSG_HELLO, MPI_COMM_WORLD);
			printf("%d: Wyslalem token w kolorze %d do %d\n", rank, c[0], (rank + 1)%size);
			resend = false; // potem to usunac
		}
	}
}

int main(int argc, char **argv)
{
	MPI_Status status;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	if (rank == 0)
	{
		msg[0] = color;
		printf("%d: Chce wyslac token o kolorze %d do %d\n", rank, color * (-1), rank + 1);
		SendToken(rank, (rank + 1)%size, msg[0]);
		MPI_Recv(msg, MSG_SIZE, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		printf("%d: Otrzymalem token o wartosci %d od %d\n", rank, msg[0], status.MPI_SOURCE);
		sender = rank;
	}
	else
	{
		//printf("%d: Wysylam token o kolorze %d do %d\n", rank, color, rank + 1);
		MPI_Recv(msg, MSG_SIZE, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		printf("%d: Otrzymalem token o wartosci %d od %d\n", rank, msg[0], status.MPI_SOURCE);
		printf("%d: Chce wyslac token o kolorze %d do %d\n", rank, color * (-1), (rank + 1)%size);
		SendToken(rank, (rank + 1)%size, msg[0]);
	}
	MPI_Finalize();
}
