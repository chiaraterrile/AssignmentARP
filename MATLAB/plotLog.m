clear all
close all
M=load ("log.txt");
figure
plot(M(:,1),M(:,2))
xlabel('timestamps [sec]')
ylabel('token value')