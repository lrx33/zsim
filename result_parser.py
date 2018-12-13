import sys

def main():
    if len(sys.argv) != 2:
        print 'Usage: %s resultfile' % sys.argv[0]
        sys.exit(1)

    rfile = sys.argv[1]
    fd = open(rfile, 'r')
    resultfile = rfile +'.result'
    print 'opening ' + resultfile

    result = open(resultfile, 'w')

    totalidle = 0
    lastout = 0

    for line in fd.read().split('\n'):
        if line == '':
            continue

        s = line.find('memacc')
        if s == -1:
            continue

        line = line[s:]
        line = line.replace(',', '')
        fields = line.split()
#        print fields,
        intime = 0
        outtime = 0
        digdone = 0

        for z in fields:
            if z.isdigit():
#                print 'z=',z,
                if digdone == 0:
                    intime = int(z)
                    digdone += 1
                elif digdone == 1:
                    outtime = int(z)
                    digdone += 1
                else:
                    raise 'wtf line: %s' % fields

        diff = intime - lastout
#        print intime, outtime, diff
        totalidle += diff
        lastout = outtime

        result.write(str(intime) + ', ' + str(totalidle) + '\n')

    result.close()
    fd.close()

if __name__ == '__main__':
    main()
