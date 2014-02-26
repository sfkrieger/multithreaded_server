#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for recv() and send() */
#include <unistd.h>     /* for close() */
#include <string.h> /* memset */
#include <time.h>
#include <pthread.h>        /* for POSIX threads */

#include "mtserver.h"  /* TCP echo server includes */
extern unsigned short cur_clients; //current number of clients
extern pthread_mutex_t lock;

void HandleTCPClient(int clntSocket) {

	//======================== VARS =========================
	char recvBuffer[BUFSIZE]; /* Buffer for echo string */
	char temp[BUFSIZE + BUFSIZE];
	int recvMsgSize; /* Size of received message */
	client_info *ci = (client_info *) malloc(sizeof(client_info));
	memset(ci, 0, sizeof(client_info)); /* Zero out structure */
	ci->sockfd = clntSocket;
	ci->total = -1;

	//-------------------- 	for select ---------------------
	int n = ci->sockfd + 1;
	int rv;
	fd_set readfds;
	struct timeval tv;
	FD_ZERO(&readfds);

	// add our descriptors to the set
	FD_SET(ci->sockfd, &readfds);
	tv.tv_sec = 5;
	tv.tv_usec = 0;

	//JUST FOR NOW: TODO: TAKE OUT!!!
	memset(ci->cur_buff, '\0', BUFSIZE);
	memset(temp, '\0', (BUFSIZE + BUFSIZE));

	int returnedVal;
	//	unsigned int start = 0;

	//======================= RECV ==========================
	/* Receive message from client */
	while ((ci->failures < 3) && (ci->exit == 0)) {

		printf("--------- New Receive----------\n");
		//JUST FOR NOW: TODO: TAKE OUT!!!
		//		memset(ci->cur_buff, '\0', BUFSIZE);
		memset(recvBuffer, '\0', BUFSIZE);
		rv = select(n, &readfds, NULL, NULL, &tv);
		if (rv == 0) {
			Shutdown(ci);
		}
		recvMsgSize = recv(clntSocket, recvBuffer, (BUFSIZE - 1), 0);
		if (recvMsgSize <= 0) {
			ci->exit = 1;
			printf("Connection terminated\n");
			break;
		}

		//-----------1. copy the info into another buffer ------

		memcpy(temp, (ci->cur_buff+ci->startIndex), ci->length);
		//TODO: take this out later!!
		printf("The temporary buffer now looks like this: %s\n", temp);
		memset(ci->cur_buff, '\0', BUFSIZE);
		//-----------1b. then copy it back... (put old things at the beginning)
		memcpy(ci->cur_buff, temp, ci->length);
		printf("After you put it back in curbuff: %s\n", ci->cur_buff);

		//-----------1c. add the new stuff to the end
		memcpy(ci->cur_buff+ci->length, recvBuffer, recvMsgSize);
		printf("This is everything now re-concatenated: %s\n", ci->cur_buff);

		//-----------1d. make updates
		ci->startIndex = 0;
		int old_len = ci->length;
		int new_len = old_len + recvMsgSize;
		ci->length = new_len;

		printf("contents of the recvBuffer: %s\n", recvBuffer);
		returnedVal = CheckIncomming(ci);
		printf("value returned %d\n", returnedVal);

	}

	Shutdown(ci);
}

/**
 * FOR EVERYTHING YOU DO, WHENEVER YOU MOVE (i++ or more) YOU NEED TO :
 * 1. update the start index
 * 2. update the length
 *
 * WHEN CHECKING THE FIRST DIGIT: double check the totals and print them
 */
int CheckIncomming(client_info *ci) {
	printf("============= RECEIVED ========= Newest:  %s\n", ci->cur_buff);
	int incomplete_str = 0; /*need to flag this variable to 0 when load, uptime, or exit can't complete for invalid chars*/
	if (ci->startIndex != 0) {
		printf("starting in the wrong place, the start index isn't = 0 \n");
	}

	while ((ci->length > 0) && (incomplete_str != -1)) {
		printf("Start index: %d, length: %d\n", ci->startIndex, ci->length);
		//iterate through the things
		char cur = ci->cur_buff[ci->startIndex];
		printf("Current char: %c\n", cur);

		//check the digits first...
		switch (cur) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			printf("servicing this digit\n");
			incomplete_str = ServiceDigit(ci);
			break;
		default:
			printf("got a non-digit character.. sum the values\n");
			if (ci->total >= 0) {
				Total(ci);
			} else {
				printf("Digits were already printed/never existed\n");
			}
			incomplete_str = ServiceChar(ci);
			break;

		}

		//end of switch, by this point we've assmed that the pointers have increments
		printf("End of main switch... \n");
	}

	printf(
			"No remaining chars to check...start index: %d, length: %d, incomplete value (-1): %d\n",
			ci->startIndex, ci->length, incomplete_str);
	return 1;
}

int ServiceChar(client_info *ci) {
	char cur = ci->cur_buff[ci->startIndex];
	int valReturned = 0;

	switch (cur) {
	case 'u':
		valReturned = ServiceUptime(ci);
		break;
	case 'l':
		valReturned = ServiceLoad(ci);
		break;
	case 'e':
		valReturned = ServiceExit(ci);
		break;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		valReturned = ServiceDigit(ci);
		break;
	default:
		printf("invalid character\n");
		valReturned = Invalid(ci); //send the response about it being invalid
		ci->startIndex++;
		ci->length--;
		break;

	} /* end of switch */

	return valReturned;

}

int Uptime(client_info *ci) {
	uint32_t cur_time = (uint32_t) time(NULL);

	//	int tmp = htonl((uint32_t)cur_time);
	printf("Timestamp: %d\n", cur_time);

	if ((send(ci->sockfd, &cur_time, 4, 0) != 4) > 0) {
		printf("send() sent a different number of bytes than expected");
		return 1;
	}

	return 1;

}
int Total(client_info *ci) {
	uint32_t sum = (uint32_t) ci->total;

	//	int tmp = htonl((uint32_t)cur_time);
	printf("Sum: %d\n", sum);

	if ((send(ci->sockfd, &sum, 4, 0) != 4) > 0) {
		printf("Worked");
		return 1;
	}
	ci->total = -1;

	return 1;

}
int Invalid(client_info *ci) {
	//it was invalid... so increment the failure count
	ci->failures++;
	int bad_req = -1;
	printf("sending a bad request message\n");
	if ((send(ci->sockfd, &bad_req, 4, 0) != 4) > 0) {
		printf("Worked");
		return 1;
	}

	if (ci->failures >= 2) {
		Shutdown(ci);
	}

	return 1;

}

int Exit(client_info *ci) {
	//want to send a 1
	uint32_t cur_time = 0;

	printf("Exit val: %d\n", cur_time);

	if ((send(ci->sockfd, &cur_time, 4, 0) != 4) > 0) {
		printf("Worked");
		return 1;
	}

	Shutdown(ci);
	return 1;

}

int Load(client_info *ci) {
	pthread_mutex_lock(&lock);
	unsigned int tmp = cur_clients;
	pthread_mutex_unlock(&lock);

	if ((send(ci->sockfd, &tmp, 4, 0) != 4) > 0) {
		printf("Worked");
		return 1;
	}

	return 1;

}

int ServiceUptime(client_info *ci) {
	int i = ci->startIndex;
	int length = ci->length;

	if (length >= 6) {
		//then it should be a valid load
		if ((ci->cur_buff[i + 1] == 'p') && (ci->cur_buff[i + 2] == 't')
				&& (ci->cur_buff[i + 3] == 'i') && (ci->cur_buff[i + 4] == 'm')
				&& (ci->cur_buff[i + 5] == 'e')) {
			printf("got uptime complete!\n");

			ci->startIndex = ci->startIndex + 6;
			ci->length = ci->length - 6;

			Uptime(ci);
			return 1;

		} else {
			// you know it matches the l
			ci->startIndex++;
			ci->length--;

			if (length >= 1) {

				if ((ci->cur_buff[ci->startIndex] == 'p')) {
					ci->length--;
					ci->startIndex++;

					if (length > 1) {

						if (ci->cur_buff[ci->startIndex] == 't') {
							ci->length--;
							ci->startIndex++;
							if ((ci->cur_buff[ci->startIndex] == 'i')) {
								ci->length--;
								ci->startIndex++;

								if (ci->cur_buff[ci->startIndex] == 'm') {
									ci->length--;
									ci->startIndex++;
									if ((ci->cur_buff[ci->startIndex] != 'e')) {
										Invalid(ci);
										return 2;
									} else {
										printf("SOmethings messed in uptime\n");
									}
								} else {
									Invalid(ci);
									return 2;
								}

							} else {

								Invalid(ci);
								return 2;
							}
						} else {

							Invalid(ci);
							return 2;
						}
					}
					//it was right but incomplete
					return -1;

				} else {
					Invalid(ci);
					return 2;
				}

			}
		}
	}

	//otherwise, its failed or its not big enough
	//want to keep the index at l if its not big enough, and update the index if it isn't move the whole thing down
	if (length > 1) { //length has to be greater than 1, otherwise its just an l
		i++;
		length--;
		//moving onto x, now were at x
		//find out the index that it fails on
		if ((ci->cur_buff[i] == 'p')) {
			i++;
			length--;

			if (length >= 1) {
				//check on it
				if (ci->cur_buff[i] == 't') {
					i++;
					length--;
					if (length >= 1) {
						if (ci->cur_buff[i] == 'i') {
							i++;
							length--;
							if (length >= 1) {

								if (ci->cur_buff[i] == 'm') {
									i++;
									length--;
									if (length >= 1) {
										if (ci->cur_buff[i] != 'e') {
											printf(
													"last char of uptime failed\n");
											ci->startIndex = i;
											ci->length = length;
											Invalid(ci);
											return -1;
										} else {
											printf(
													"This shouldn't be happening.. in uptime\n");
										}

									} else {
										return -1;
									}

								} else {
									printf("uptime failed\n");
									ci->startIndex = i;
									ci->length = length;
									Invalid(ci);
									return -1;
								}

							} else {
								//not large enough for i
								return -1;
							}
						} else {
							printf("uptime failed\n");
							ci->startIndex = i;
							ci->length = length;
							Invalid(ci);
							return -1;
						}

					} else {
						//not large enough for i
						return -1;
					}

				} else {
					//then failed the i
					printf("Failed t in uptime\n");
					ci->startIndex = i;
					ci->length = length;
					Invalid(ci);
					return -2;
				}
			} else {
				return -1;
			}

		} else { //next element wasn't an p
			ci->startIndex++;
			ci->length++;
			Invalid(ci);
			return -2;
		}

	} else {
		//it was just the l, nothing else so don't change the index, just return it as is
		return -2;
	}

	printf("invalid control flow in service load\n");
	return -5;

}
// 1 is everything is fine, 2, is it didn't work but there was enough space, -1 is it was incomplete and -2 it was incomplete and wrong
int ServiceLoad(client_info *ci) {
	int i = ci->startIndex;
	int length = ci->length;
	//	int endIndex = (i + ci->length) - 1;
	//	char cur = ci->cur_buff[i];

	if (length >= 4) {
		//then it should be a valid load
		if ((ci->cur_buff[i + 1] == 'o') && (ci->cur_buff[i + 2] == 'a')
				&& (ci->cur_buff[i + 3] == 'd')) {
			printf("got load complete!\n");
			ci->startIndex = ci->startIndex + 4;
			ci->length = ci->length - 4;
			Load(ci);
			return 1;
		} else {
			// you know it matches the l
			ci->startIndex++;
			ci->length--;

			if ((ci->cur_buff[ci->startIndex] == 'o')) {
				ci->length--;
				ci->startIndex++;
				if (ci->cur_buff[ci->startIndex] == 'a') {
					ci->length--;
					ci->startIndex++;
					if ((ci->cur_buff[ci->startIndex] != 'd')) {
						Invalid(ci);
						return 2;
					}
					printf(
							"terrible control flow in load, when string is bigger than 4");
				} else {

					Invalid(ci);
					return 2;
				}
			} else {

				Invalid(ci);
				return 2;
			}

			Invalid(ci);
			return 2;
		}

	}

	//otherwise, its failed or its not big enough
	//want to keep the index at l if its not big enough, and update the index if it isn't move the whole thing down
	if (length > 1) { //length has to be greater than 1, otherwise its just an l
		i++;
		length--;
		//moving onto x, now were at x
		//find out the index that it fails on
		if ((ci->cur_buff[i] == 'o')) {
			i++;
			length--;
			//moving onto i

			if (length >= 1) {
				//check on it
				if (ci->cur_buff[i] == 'a') {
					i++;
					length--;
					if (length >= 1) {
						if (ci->cur_buff[i] == 'd') {
							printf(
									"this is definitely a problem, each index looks like it isn't right but it is?\n");
						} else {
							printf("last char of load failed\n");
							ci->startIndex = i;
							ci->length = length;
							Invalid(ci);
							return -1;
						}

					} else {
						//not large enough for i
						return -1;
					}

				} else {
					//then failed the i
					printf("Failed a in load\n");
					ci->startIndex = i;
					ci->length = length;
					Invalid(ci);
					return -2;
				}
			} else { //failed the l
				//dont need to move anything
				return -1;
			}

		} else { //next element wasn't an o
			ci->startIndex++;
			ci->length++;
			Invalid(ci);
			return -2;
		}

	} else {
		//it was just the l, nothing else so don't change the index, just return it as is
		return -2;
	}

	printf("invalid control flow in service load\n");
	return -5;

}

int ServiceExit(client_info *ci) {
	int i = ci->startIndex;
	int length = ci->length;
	//	int endIndex = (i + ci->length) - 1;
	//	char cur = ci->cur_buff[i];

	if (length >= 4) {
		//then it should be a valid load
		if ((ci->cur_buff[i + 1] == 'x') && (ci->cur_buff[i + 2] == 'i')
				&& (ci->cur_buff[i + 3] == 't')) {
			printf("got exit complete!\n");
			ci->startIndex = ci->startIndex + 4;
			ci->length = ci->length - 4;
			Exit(ci);
			return 1;
		} else {
			// you know it matches the l
			ci->startIndex++;
			ci->length--;

			if ((ci->cur_buff[ci->startIndex] == 'x')) {
				ci->length--;
				ci->startIndex++;
				if (ci->cur_buff[ci->startIndex] == 'i') {
					ci->length--;
					ci->startIndex++;
					if ((ci->cur_buff[ci->startIndex] != 't')) {
						Invalid(ci);
						return 2;
					}
					printf(
							"terrible control flow in load, when string is bigger than 4");
				} else {

					Invalid(ci);
					return 2;
				}
			} else {

				Invalid(ci);
				return 2;
			}

			Invalid(ci);
			return 2;
		}

	}

	//otherwise, its failed or its not big enough
	//want to keep the index at l if its not big enough, and update the index if it isn't move the whole thing down
	if (length > 1) { //length has to be greater than 1, otherwise its just an l
		i++;
		length--;
		//moving onto x, now were at x
		//find out the index that it fails on
		if ((ci->cur_buff[i] == 'x')) {
			i++;
			length--;
			//moving onto i

			if (length >= 1) {
				//check on it
				if (ci->cur_buff[i] == 'i') {
					i++;
					length--;
					if (length >= 1) {
						if (ci->cur_buff[i] == 't') {
							printf(
									"this is definitely a problem, each index looks like it isn't right but it is?\n");
						} else {
							printf("last char of exit failed\n");
							ci->startIndex = i;
							ci->length = length;
							Invalid(ci);
							return -1;
						}

					} else {
						//not large enough for i
						return -1;
					}

				} else {
					//then failed the i
					printf("Failed a in load\n");
					ci->startIndex = i;
					ci->length = length;
					Invalid(ci);
					return -2;
				}
			} else { //failed the l
				//dont need to move anything
				return -1;
			}

		} else { //next element wasn't an o
			ci->startIndex++;
			ci->length++;
			Invalid(ci);
			return -2;
		}

	} else {
		//it was just the l, nothing else so don't change the index, just return it as is
		return -2;
	}

	printf("invalid control flow in service exit\n");
	return -5;
}
/**
 * Increments the index
 * Checks the current index, converts it to a digit, adds it to the running total, and returns 1
 */
int ServiceDigit(client_info *ci) {
	char cur = ci->cur_buff[ci->startIndex];
	if (ci->total == -1) {
		ci->total = 0;
		printf("Looks like we've hit out first digit\n");
	}

	//check the current one
	int val = cur - '0';
	ci->total = ci->total + val;
	ci->startIndex++;
	ci->length--;
	return 1;
}

int Shutdown(client_info *ci) {
	//===================== SHUTDOWN ========================
	printf("Closing connection\n");
	close(ci->sockfd); /* Close client socket */
	pthread_mutex_lock(&lock);
	cur_clients = cur_clients - 1;
	pthread_mutex_unlock(&lock);
	int a = 3;
	free(ci);
	pthread_exit(&a);

	//decrement global number of connected clients
	//kill current running process

}
