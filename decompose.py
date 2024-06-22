import itertools
import re
import sys
import os

startKey = ['library', 'use']
ban = ['clk', 'sel']
outIndex = ['Q','data_out','q0','output']
all_cases = []


# modules = {'moduleName': {'INPUT': ['a', 'b'], 'OUTPUT': ['c']}
def get_mux_info_work(fileName, mux):
    modules = {}
    module = []
    isReadingModule = False
    getIOInfo = False
    currentModuleName = ''

    with open(fileName, 'r') as vhdFile:
        for line in vhdFile:
            words = line.split()
            # print(words)
            if len(words) == 0:
                continue
            # if 'work.' in line:
            #     print(line.split('work.')[1].split('\n')[0] + "==================   " + mux)
            if 'work.' in line and line.split('work.')[1].split('\n')[0] in mux and not '--' in line:
                # print(line.split('work.')[1])
                isReadingModule = True
                currentModuleName = words[0].split(':')[0]
                print('current module name is:  ' + currentModuleName)
                modules[currentModuleName] = {'INPUT': [], 'OUTPUT': []}
                module.append(line)
                continue

            if isReadingModule:
                module.append(line)

            if ';' in words[-1] and isReadingModule:

                print('Now analysis the module: \n')
                isReadingModule = False
                for index in module:
                    if getIOInfo:
                        beforeIndex = index.split('=>')[0].strip()
                        afterIndex = index.split('=>')[1].split(',')[0].split(')')[0]

                        if beforeIndex in ban:
                            continue
                        elif beforeIndex in outIndex:
                            modules[currentModuleName]['OUTPUT'].append(afterIndex.strip())
                        else:
                            modules[currentModuleName]['INPUT'].append(afterIndex.strip())

                    if 'PORT' in index:
                        print('analysis the port map')
                        getIOInfo = True

                getIOInfo = False
                module = []

    return modules


def get_mux_info_module(fileName, mux):
    modules = {}
    module = []
    isReadingModule = False
    getIOInfo = False
    currentModuleName = ''

    with open(fileName, 'r') as vhdFile:
        for line in vhdFile:
            words = line.split()
            # print(words)
            if len(words) == 0:
                continue
            # if 'work.' in line:
            #     print(line.split('work.')[1].split('\n')[0] + "==================   " + mux)
            if 'work.' in line and ':' in line and line.split(':')[0].strip() in mux and not '--' in line:
                # print(line.split('work.')[1])
                isReadingModule = True
                currentModuleName = words[0].split(':')[0]
                print('Current module name is:  ' + currentModuleName)
                modules[currentModuleName] = {'INPUT': [], 'OUTPUT': []}
                module.append(line)
                continue

            if isReadingModule:
                module.append(line)

            if ';' in words[-1] and isReadingModule:

                print('Now analysis the module... \n')
                isReadingModule = False
                for index in module:
                    if getIOInfo:
                        beforeIndex = index.split('=>')[0].strip()
                        afterIndex = index.split('=>')[1].split(',')[0].split(')')[0]

                        if beforeIndex in ban:
                            continue
                        elif beforeIndex in outIndex:
                            modules[currentModuleName]['OUTPUT'].append(afterIndex.strip())
                        else:
                            modules[currentModuleName]['INPUT'].append(afterIndex.strip())

                    if 'PORT' in index:
                        # print('Analysis the port map')
                        getIOInfo = True

                getIOInfo = False
                module = []

    return modules


def generate_mux_combinations(data_dict):
    # get all combinations of the muxes
    mux_combinations = {mux: list(itertools.product(io['INPUT'], [io['OUTPUT']])) 
                        for mux, io in data_dict.items()}

    # Generate all combinations of the muxes
    keys, values = zip(*mux_combinations.items())
    combinations = [dict(zip(keys, v)) for v in itertools.product(*values)]

    # Transform the combinations into the format that can be used by the parser
    formatted_combinations = []
    for combo in combinations:
        formatted_combo = {mux: {'INPUT': in_out[0], 'OUTPUT': in_out[1]}
                           for mux, in_out in combo.items()}
        formatted_combinations.append(formatted_combo)

    return formatted_combinations


def remove_comments(input_file):
    output_file = f'file_removed_comments.vhd'
    try:
        with open(input_file, 'r') as inputFile:
            with open(output_file, 'w') as outputFile:
                for line in inputFile:
                    filtered_line = line.split('--')[0]
                    if line.startswith('--'):
                        continue
                    if not filtered_line.endswith('\n'):
                        filtered_line += '\n'
                    outputFile.writelines(filtered_line)

        return output_file

    except FileNotFoundError:
        print("Failed to remove the comments from the file: " + input_file + "...\n")



'''
    INPUT FORMAT:
    python3 vhd_decompose.py <source_vhd_file> <mux_module_name>
'''
if __name__ == '__main__':
    print("Now start to decompose...\n")

    # input arguments
    arguments = sys.argv
    vhdFileName = arguments[1]
    muxModule = []

    for i in range(2, len(arguments)):
        muxModule.append(arguments[i])

    mudDict = {}
    num = 0
    count = 0
    file_name = ''
    startComment = False

    print("Now we start to analyze the file: " + vhdFileName + "...\n")

    muxDict = get_mux_info_module(vhdFileName, muxModule)
    print("============================================================")
    print("We get all mux modules in the file: \n")
    print(muxDict)
    print("============================================================")

    combinationDict = generate_mux_combinations(muxDict)
    print("We get all mux combinations in the file: \n")
    for combination in combinationDict:
        count += 1
        print("The " + str(count) + "st combination is ")
        print(combination)
        print('\n')
    print("============================================================")

    print("Exchange the mux combinations into the format that can be used by the parser...\n")

    for pair in combinationDict:
        sortedPair = dict(sorted(pair.items(), key=lambda item: len(item[1]['OUTPUT']), reverse=True))
        file_name = f'target_file_{num}.txt'
        num += 1
        print(f'Now start to generate {file_name}...\n')
        with open(vhdFileName, 'r') as vhdFile:
            with open(file_name, 'w') as targetFile:
                for line in vhdFile:
                    words = line.split()
                    if len(words) == 0:
                        continue

                    startIndex = line.strip().split(':')[0]
                    if startIndex in sortedPair.keys():
                        # print('Now start to add the comments...\n')
                        startComment = True

                    if not startComment:
                        writeLine = line
                        for key in sortedPair.keys():
                            if sortedPair[key]['OUTPUT'][0] in line and not 'signal' in line:
                                writeLine = writeLine.replace(sortedPair[key]['OUTPUT'][0], sortedPair[key]['INPUT'])
                                # print("We can replace: " + writeLine)
                                # print("And the origin is " + line + '\n')
                        targetFile.write(writeLine)
                    else:
                        targetFile.write(' -- ' + line)
                        # print(line)
                        if line.split('\n')[0].endswith(';'):
                            # print('Finish the comments...\n')
                            startComment = False
    # file_removed_comments = remove_comments(vhdFileName)
    # # os.remove(file_removed_comments)
    # print(file_removed_comments)


    # get_mux_info(vhdFileName, muxModule)
