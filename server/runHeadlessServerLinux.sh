./makeLogRotator
nohup catchsegv unbuffer ./CrucibleServer | ./logRotator 20000000 serverOut.txt >/dev/null &