cd /Users/akyrola/Projects/pbbs/trunk/blacklight_study/numa_testsuite/results

file = 'access_par_apr26.csv';
d = importdata(file);
d = d.data;


CPU = 2;
TIME = 3;
TARGET = 4;

cpus = unique(d(:,CPU));
d = sortrows(d, TARGET);

cpus = [0,32,64,96]
    markers='x^<+*sdv^<>ph'
    colors='bgcrbgrcy'
hold off
for i = 1:length(cpus)
    c = cpus(i);
    fprintf('Plotting %d\n', c)
    rows = find(d(:,CPU) == c);

    markt = sprintf('%s-%s', colors(i),markers(i)); 
    
    plot(d(rows, TARGET), d(rows, TIME), markt,  'LineWidth',2)
    hold on
end


title('PARALLEL: Time to READ a 128 mb segment data ')
legend('CPU id:0', 'CPU id:32', 'CPU id:64', 'CPU id:96')
xlabel('Target CPU')
ylabel('Seconds')

saveas( gcf, 'blacklight_par_access.pdf', 'pdf')