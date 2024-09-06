client: 
	gcc ./Client/client.c -o ./Client/client -lcjson

runClient:
	./Client/client

server: 
	gcc ./Server/server.c -o ./Server/server -lcjson

runServer:
	./Server/server

cleanClinet: 
	rm -rf ./Client/client

cleanServer:
	rm -rf ./Server/server

