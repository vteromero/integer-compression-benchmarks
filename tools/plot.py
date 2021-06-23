import json
import matplotlib.pyplot as plt
import re
import sys

def add_to_dict(d, name, speed, ratio):
    if name in d:
        d[name]['speed'].append(speed)
        d[name]['ratio'].append(ratio)
    else:
        d[name] = {'speed': [speed], 'ratio': [ratio]}

def plot_enc_speed_vs_ratio(data):
    marker_by_name = {
        'VTEnc':                '-v',
        'DeltaVariableByte':    '-^',
        'DeltaVarIntGB':        '-*',
        'DeltaBinaryPacking':   '-s',
        'DeltaFastPFor128':     '-d',
        'DeltaFastPFor256':     '-D'
    }

    for name, values in data.items():
        plt.plot(values['speed'], values['ratio'], marker_by_name[name], label=name)

    plt.title('Encoding speed vs ratio')
    plt.xlabel('Encoding speed (MB/s)')
    plt.ylabel('Encoding ratio (smaller is better)')
    plt.legend(numpoints=1)
    plt.grid(True)
    plt.show()

def plot_dec_speed(data):
    for name, values in data.items():
        plt.bar(name, values['speed'])

    plt.title('Decoding speed (MB/s)')
    plt.grid(True)
    plt.show()

encoding_dict = {}
decoding_dict = {}

with open(sys.argv[1]) as json_file:
    json_data = json.load(json_file)

    for b in json_data['benchmarks']:
        raw_name = b['name']
        full_name = raw_name.split('/')[1]
        name = re.sub(r'Decode$', '', re.sub(r'Encode$', '', full_name))

        bytes_per_second = b['bytes_per_second']
        megas_per_second = (bytes_per_second / 1024.0) / 1024.0

        compression_ratio = b['compressionRatio']
        print(raw_name, full_name, name, megas_per_second, compression_ratio)

        if full_name.endswith('Encode'):
            add_to_dict(encoding_dict, name, megas_per_second, compression_ratio)
        elif full_name.endswith('Decode'):
            add_to_dict(decoding_dict, name, megas_per_second, compression_ratio)

    print(encoding_dict)
    print(decoding_dict)

    plot_enc_speed_vs_ratio(encoding_dict)
    plot_dec_speed(decoding_dict)
