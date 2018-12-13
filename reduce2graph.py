import sys

def main():
    if len(sys.argv) != 2:
        print 'Usage: %s resultfile' % sys.argv[0]
        sys.exit(1)

    rfile = sys.argv[1]
    if not '.result' in rfile:
        print 'No .result in %s' % rfile
        sys.exit(2)

    fd = open(rfile, 'r')
    resultfile = rfile +'.reduced'
    print 'opening ' + resultfile

    result = open(resultfile, 'w')

    numpoints = 100
    points = []

    for line in fd.read().split('\n'):
        if line == '':
            continue

        fields = line.split(',')
        points.append((int(fields[0]), int(fields[1])))

    xmax = points[-1][0]
    xmin = points[0][0]

    step = (xmax - xmin) / numpoints
    curval = xmin

    for point in points:
        if point[0] >= curval:
            print point
            result.write(str(point[0]) + ', ' + str(point[1]) + '\n')
            curval += step

    result.close()
    fd.close()

if __name__ == '__main__':
    main()
