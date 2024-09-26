client: 
	gcc ./Client/client.c -o ./build/client -lcjson

server: 
	gcc ./Server/server.c -o ./build/server -lcjson

runServer:
	./build/server

runClient:
	./build/client

clean: 
	rm -rf ./build/client
	rm -rf ./build/server

