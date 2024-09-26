client: 
	gcc ./Client/client.c -o ./build/client -lcjson

runClient:
	./Client/client

server: 
	gcc ./Server/server.c -o ./build/server -lcjson

runServer:
	./Server/server

cleanClinet: 
	rm -rf ./Client/
	

cleanServer:
	rm -rf ./Server/server

