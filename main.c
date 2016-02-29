/*!
 * \brief	Данная программа демонстрирует межпроцессное взаимодействие.
 *
 * \author	Рогоза А. А.
 * \author	Романов С. А.
 * \date	28/02/2016
 */
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>

#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include <sys/stat.h>

const int SIGNAL_NEED_STOP = SIGUSR1;
const int SIGNAL_ENDED		= SIGUSR2;

int file;	//! file descriptor
int fd[2];	//! read/write

int id_p1,
	id_p2;

/*!
 * \brief Структура для передачи сообщений по каналу
 */
struct ProcessInfo
{
	uint32_t	id;
	char		message[40];
} processInfo;

/*!
 * \brief Выводит сообщение об ошибке и аварийно завершает программу.
 * \param errorString Строка для вывода.
 */
void printError()
{
	sprintf( processInfo.message, "Error while opeping the file\n" );
	write( fd[1], &processInfo, sizeof( processInfo ) );
	exit( 1 );
}

/*!
 * \brief Прерывает чтение файла во 2-ом процессе.
 * \param n Фиктивный параметр.
 */
void stopReadFile( int n )
{
	processInfo.id = id_p2;
	sprintf( processInfo.message, "Reading file has been stopped\n" );
	write( fd[1], &processInfo, sizeof( processInfo ) );
	exit(0);
}

int main()
{
	printf( "Process 0 was created.\n" );

	//! Создание канала для кооперации между процессами.
	pipe( fd );

	int id_p2;
	int file2;

	//! Создание первого процесса.
	int id_p1 = fork();
	if ( id_p1 == 0 ) {
		//! p1 process

		//! Создание второго процесса.
		id_p2 = fork();
		if ( id_p2 == 0 ) {
			//! p2 process
			file = open( "input.txt", O_RDONLY );
			file2 = open( "log.txt", O_WRONLY | O_CREAT, 0644 );
			if ( file != -1  ) {
				//! reading the file
				signal( SIGNAL_NEED_STOP, stopReadFile );

				char c;
				//! reading file;
				while ( read( file, &c, sizeof( char ) ) )
					write( file2, &c, sizeof( char ) );
			}
			else
				printError();
		}
		else  {
			//! p1
			sprintf( processInfo.message, "Process 2 has been created.\n" );
			processInfo.id = id_p1;

			//! Запись в канал сообщения о создании 2-го процесса.
			write( fd[1], &processInfo, sizeof( struct ProcessInfo ) );
			sleep( 3 );

			//! Отправка сообщения с целью остановки чтения.
			kill( id_p2, SIGNAL_NEED_STOP );

			int errorStatus;
			wait( &errorStatus );
			if ( WIFEXITED( errorStatus ) != 0 ) {
				sprintf( processInfo.message, "File is too small\n" );
				write( fd[1], &processInfo, sizeof( processInfo ) );
			}

			//! Запись в канал сообщения о закрытии 2-го процесса.
			sprintf( processInfo.message, "Process 2 has been closed.\n" );
			write( fd[1], &processInfo, sizeof( struct ProcessInfo ) );

			exit( 0 );
		}
	}
	else {
		//! p0
		printf( "Process 1 has been created!\n" );

		//! Чтение из канала
		read( fd[0], &processInfo, sizeof( processInfo ) );
		printf( "%s\n", processInfo.message );

		read( fd[0], &processInfo, sizeof( processInfo ) );
		printf( "%s\n", processInfo.message );

		read( fd[0], &processInfo, sizeof( processInfo ) );
		printf( "%s\n", processInfo.message );

		int errorStatus;
		wait( &errorStatus );

		printf( "Process 1 has been closed!\n" );
		printf( "Process 0 is closing.\n" );
	}
	return 0;
}
