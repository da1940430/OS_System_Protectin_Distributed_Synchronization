all:ftp_server ftp_client generate_100M

ftp_server:ftp_server.c server_function.c
	gcc -g ftp_server.c server_function.c -o ./out/ftp_server -D_GNU_SOURCE -pthread

ftp_client:ftp_client.c client_function.c 
	gcc -g ftp_client.c client_function.c -o ./out/ftp_client -D_GNU_SOURCE

generate_100M:generate_100M.c 
	gcc -g generate_100M.c -o ./out/generate_100M

clean:
	rm ./out/ftp_server ./out/ftp_client ./out/generate_100M

