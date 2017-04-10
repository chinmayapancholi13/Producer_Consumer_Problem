manager: manager.c producer consumer
	gcc -o manager manager.c

producer: producer.c
	gcc -o producer producer.c

consumer: consumer.c
	gcc -o consumer consumer.c

clean:
	rm manager producer consumer matrix.txt results.txt

run: ./manager