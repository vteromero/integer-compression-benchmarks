#!/usr/bin/env python3

import json
import matplotlib.pyplot as plt
import re
import argparse

def plot_speed_vs_comp_ratio(data, type_word):
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

    plt.title('{} speed vs compression ratio'.format(type_word))
    plt.xlabel('{} speed (MB/s)'.format(type_word))
    plt.ylabel('Compression ratio (bigger is better)')
    plt.legend(numpoints=1)
    plt.grid(True)
    plt.show()

def plot_speed(data, type_word):
    for name, values in data.items():
        plt.bar(name, values['speed'])

    plt.title('{} speed (MB/s)'.format(type_word))
    plt.grid(True)
    plt.show()

def add_to_dict(d, name, speed, ratio):
    if name in d:
        d[name]['speed'].append(speed)
        d[name]['ratio'].append(ratio)
    else:
        d[name] = {'speed': [speed], 'ratio': [ratio]}

def parse_file(filename):
    encoding_dict = {}
    decoding_dict = {}

    with open(filename) as json_file:
        json_data = json.load(json_file)

        for b in json_data['benchmarks']:
            raw_name = b['name']
            full_name = raw_name.split('/')[1]
            name = re.sub(r'Decode$', '', re.sub(r'Encode$', '', full_name))

            bytes_per_second = b['bytes_per_second']
            megas_per_second = (bytes_per_second / 1024.0) / 1024.0

            compression_ratio = b['compressionRatio']

            if full_name.endswith('Encode'):
                add_to_dict(encoding_dict, name, megas_per_second, compression_ratio)
            elif full_name.endswith('Decode'):
                add_to_dict(decoding_dict, name, megas_per_second, compression_ratio)

    return encoding_dict, decoding_dict

plot_type_to_function = {
    'speed_vs_ratio': plot_speed_vs_comp_ratio,
    'speed': plot_speed
}

plot_types = list(plot_type_to_function.keys())

def run(args):
    encoding_data, decoding_data = parse_file(args.filename)
    plot_func = plot_type_to_function[args.plottype]

    if args.encoding:
        plot_func(encoding_data, 'Encoding')
    else:
        plot_func(decoding_data, 'Decoding')

parser = argparse.ArgumentParser(description='Parses benchmark output json file to plot results')
parser.add_argument('-f', '--json-file', dest='filename', metavar='',
                    help='benchmarks output file (json)', required=True)
parser.add_argument('-p', '--plot-type', choices=plot_types,
                    dest='plottype', metavar='', required=True,
                    help='plot type: {}'.format(plot_types))
group = parser.add_mutually_exclusive_group(required=True)
group.add_argument('-e', '--encoding', action='store_true', help='show encoding graph')
group.add_argument('-d', '--deconding', action='store_true', help='show decoding graph')
args = parser.parse_args()

run(args)

