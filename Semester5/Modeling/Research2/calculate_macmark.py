"""Calculate variant 32/32/30 through the MacMark stdio MCP server."""
from pathlib import Path
import argparse
import csv
import json
import re
import subprocess
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import system1
import system2
from solve import linear

ROOT = Path(__file__).resolve().parent
OUT = ROOT / 'results' / 'macmark'
DEFAULT_SERVER = Path.home() / 'Personal/MacMark/macmark-rs/target/release/macmark-mcp'
PLOT_KEYS = [('total_loss', 'Вероятность потери'),
             ('total_throughput', 'Производительность, 1/с'),
             ('total_busy', 'Среднее число занятых приборов'),
             ('total_queue', 'Средняя суммарная очередь'),
             ('total_wait', 'Среднее ожидание, с'),
             ('total_sojourn', 'Среднее пребывание, с')]
SCALARS = {'total_busy': 'Busy', 'total_queue': 'Queue', 'total_number': 'Number',
           'total_wait': 'Wait', 'total_sojourn': 'Sojourn', 'total_loss': 'Loss',
           'total_throughput': 'Throughput', 'total_load': 'Load', 'idle': 'Idle'}
PAIRS = {'busy': 'Busy', 'queue': 'Queue', 'number': 'Number', 'wait': 'Wait',
         'sojourn': 'Sojourn', 'loss': 'Loss', 'throughput': 'Throughput',
         'offered_load': 'Load'}


class MacMark:
    def __init__(self, executable):
        self.process = subprocess.Popen([str(executable)], stdin=subprocess.PIPE,
                                        stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                        text=True, bufsize=1)
        self.requests = []
        self.sequence = 0
        self.server = self.rpc('initialize', {
            'protocolVersion': '2025-11-25', 'capabilities': {},
            'clientInfo': {'name': 'research2', 'version': '1.0'}})['serverInfo']
        self.process.stdin.write(json.dumps({'jsonrpc': '2.0', 'method': 'notifications/initialized'}) + '\n')
        self.process.stdin.flush()
        self.rpc('tools/list', {})

    def rpc(self, method, params):
        self.sequence += 1
        request = {'jsonrpc': '2.0', 'id': self.sequence, 'method': method, 'params': params}
        self.process.stdin.write(json.dumps(request, ensure_ascii=False) + '\n')
        self.process.stdin.flush()
        line = self.process.stdout.readline()
        if not line:
            raise RuntimeError('MacMark closed stdout: ' + self.process.stderr.read())
        response = json.loads(line)
        self.requests.append({'request': request, 'response': response})
        if 'error' in response:
            raise RuntimeError(response['error'])
        return response['result']

    def call(self, name, arguments):
        result = self.rpc('tools/call', {'name': name, 'arguments': arguments})
        if result.get('isError'):
            raise RuntimeError(result['content'])
        return json.loads(result['content'][0]['text'])

    def close(self):
        self.process.stdin.close()
        self.process.wait(timeout=10)
        if self.process.returncode != 0:
            raise RuntimeError(self.process.stderr.read())


def formulas(index):
    """State rewards and arrival-conditioned times, never Little ratios."""
    if index == 1:
        busy1 = linear([0, 1, 0] + [1] * 8)
        busy2 = linear([0, 0, 1] + [1] * 8)
        queue = linear([0, 0, 0] + list(range(8)))
        waiting = linear([0, 0, 0] + list(range(1, 8)) + [0]) + '/(2*Mu)'
        queued_admission = linear([0, 0, 0] + [1] * 7 + [0])
        alpha1 = f'(0.65*p0+p2+0.5*{queued_admission})'
        alpha2 = f'(0.35*p0+p1+0.5*{queued_admission})'
        items = [('Busy1', busy1), ('Busy2', busy2), ('Busy', 'Busy1+Busy2'),
                 ('Queue', queue), ('Number', 'Busy+Queue'), ('Loss', 'p10'),
                 ('Throughput1', 'Mu*Busy1'), ('Throughput2', 'Mu*Busy2'),
                 ('Throughput', 'Throughput1+Throughput2'),
                 ('Wait', f'({waiting})/(1-Loss)'),
                 ('Wait1', f'({waiting})/(2*{alpha1})'),
                 ('Wait2', f'({waiting})/(2*{alpha2})'),
                 ('Sojourn1', 'Wait1+1/Mu'), ('Sojourn2', 'Wait2+1/Mu'),
                 ('Sojourn', 'Wait+1/Mu')]
    else:
        n1 = np.repeat(np.arange(5), 7)
        n2 = np.tile([0, 1, 1, 1, 2, 2, 2], 5)
        phase = np.tile([0, 1, 2, 3, 1, 2, 3], 5)
        items = [('Busy1', linear(n1 > 0)), ('Busy2', linear(n2 > 0)),
                 ('Busy', 'Busy1+Busy2'),
                 ('Queue1', linear(np.maximum(n1 - 1, 0))),
                 ('Queue2', linear(np.maximum(n2 - 1, 0))),
                 ('Queue', 'Queue1+Queue2'), ('Number1', 'Busy1+Queue1'),
                 ('Number2', 'Busy2+Queue2'), ('Number', 'Number1+Number2'),
                 ('Loss1', linear(n1 == 4)), ('Loss2', linear(n2 == 2)),
                 ('Loss', '0.65*Loss1+0.35*Loss2'),
                 ('Throughput1', 'Mu*Busy1'),
                 ('Throughput2', f'Nu*{linear((n2 > 0) & (phase == 3))}'),
                 ('Throughput', 'Throughput1+Throughput2'),
                 ('Wait1', f'{linear(n1*(n1<4))}/Mu/(1-Loss1)'),
                 ('Wait2', f'{linear((4-phase)*(n2==1))}/Nu/(1-Loss2)'),
                 ('Sojourn1', 'Wait1+1/Mu'), ('Sojourn2', 'Wait2+3/Nu'),
                 ('Wait', '(0.65*(1-Loss1)*Wait1+0.35*(1-Loss2)*Wait2)/(1-Loss)'),
                 ('Sojourn', '(0.65*(1-Loss1)*Sojourn1+0.35*(1-Loss2)*Sojourn2)/(1-Loss)')]
    items += [('Load1', 'Lam*0.65/Mu'),
              ('Load2', 'Lam*0.35/' + ('Mu' if index == 1 else '(Nu/3)')),
              ('Load', 'Load1+Load2'), ('Idle', 'p0')]
    return [{'name': name, 'formula': expr} for name, expr in items]


def numeric_formulas(items):
    values = {'Lam': '0.5', 'Mu': '0.1', 'Nu': '0.3'}
    return [{'name': item['name'], 'formula': re.sub(
        r'\b(Lam|Mu|Nu)\b', lambda m: values[m[0]], item['formula'])} for item in items]


def metrics(index, values):
    result = {key: float(values[name]) for key, name in SCALARS.items()}
    for key, prefix in PAIRS.items():
        result[key] = (None if index == 1 and key in ('queue', 'number', 'loss') else
                       [float(values[prefix + '1']), float(values[prefix + '2'])])
    return result


def named_transitions(index, q):
    transitions = []
    for a in range(len(q)):
        for b in range(len(q)):
            if a == b or q[a, b] == 0:
                continue
            if index == 1:
                if a == 0:
                    name, coefficient = 'Lam', (0.65 if b == 1 else 0.35)
                elif b > a:
                    name, coefficient = 'Lam', 1
                else:
                    name, coefficient = 'Mu', (2 if a >= 4 else 1)
            elif b // 7 > a // 7:
                name, coefficient = 'Lam', 0.65
            elif b // 7 < a // 7:
                name, coefficient = 'Mu', 1
            elif (a % 7, b % 7) in ((0, 1), (1, 4), (2, 5), (3, 6)):
                name, coefficient = 'Lam', 0.35
            else:
                name, coefficient = 'Nu', 1
            expected = {'Lam': .5, 'Mu': .1, 'Nu': .3}[name] * coefficient
            assert abs(expected - q[a, b]) < 1e-14
            transitions.append({'from': a, 'to': b, 'value': name, 'coefficient': coefficient})
    return transitions


def save_json(name, value):
    (OUT / name).write_text(json.dumps(value, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--server', type=Path, default=DEFAULT_SERVER)
    args = parser.parse_args()
    OUT.mkdir(parents=True, exist_ok=True)
    client = MacMark(args.server)
    data = {'parameters': {'lambda': .5, 'b': 10, 'p1': .65, 'p2': .35},
            'source': {'server': client.server, 'transport': 'MCP stdio', 'tool': 'solve_chain'},
            'systems': [], 'verification': {}}
    curves = []
    try:
        for index, module in enumerate((system1, system2), 1):
            reference = module.analyze()
            q = reference['Q']
            ff = formulas(index)
            rates = [{'from': a, 'to': b, 'rate': float(q[a, b])}
                     for a in range(len(q)) for b in range(len(q)) if a != b and q[a, b] != 0]
            # Inline solving preserves the exact decimal inputs, independently of
            # the legacy .mrk format's f32 serialization of elements/coefficients.
            exact = client.call('solve_chain', {'states': len(q), 'rates': rates,
                                               'formulas': numeric_formulas(ff)})
            pi = np.array([exact['probabilities'][f'p{i}'] for i in range(len(q))])
            mm = metrics(index, exact['characteristics'])
            differences = [abs(mm[key] - value) for key, value in reference['metrics'].items()
                           if isinstance(value, (int, float))]
            differences += [abs(a-b) for key in PAIRS if mm[key] is not None
                            for a,b in zip(mm[key], reference['metrics'][key])]
            checks = {'probability_difference': float(np.max(np.abs(pi-reference['pi']))),
                      'characteristic_difference': max(differences),
                      'stationary_residual': float(np.max(np.abs(pi@q))),
                      'normalization': float(abs(pi.sum()-1)),
                      'little_queue_check_only': abs(mm['total_queue']-mm['total_throughput']*mm['total_wait']),
                      'little_number_check_only': abs(mm['total_number']-mm['total_throughput']*mm['total_sojourn'])}
            checks['flow_conservation'] = abs(mm['total_throughput']-.5*(1-mm['total_loss']))
            if index == 2:
                checks['per_server_flow'] = max(
                    abs(rate*(1-loss)-completed)
                    for rate,loss,completed in zip((.325,.175),mm['loss'],mm['throughput']))
            assert max(checks.values()) < 1e-10, checks
            data['systems'].append({'states': reference['states'], 'Q': q.tolist(),
                                    'pi': pi.tolist(), 'metrics': mm})
            path = str(OUT / f'system{index}.mrk')
            client.call('create_model', {'path': path, 'states': len(q), 'overwrite': True,
                         'elements': [{'name': 'Lam', 'value': .5}, {'name': 'Mu', 'value': .1}]
                                     + ([{'name': 'Nu', 'value': .3}] if index == 2 else [])})
            client.call('set_user_info', {'path': path, 'first_name': 'Роман Вячеславович',
                                         'last_name': 'Тимошкин', 'group': 'P3330', 'date': '24.10.2026'})
            client.call('set_transitions', {'path': path, 'clear': True,
                                            'transitions': named_transitions(index, q)})
            native = client.call('set_formulas', {'path': path, 'formulas': ff})
            if 'compute_error' in native:
                raise RuntimeError(native['compute_error'])
            native = client.call('compute', {'path': path})
            native_pi = np.array([native['probabilities'][f'p{i}'] for i in range(len(q))])
            checks['mrk_probability_difference'] = float(np.max(np.abs(native_pi-pi)))
            checks['mrk_characteristic_difference'] = max(abs(float(native['characteristics'][key])-value)
                                                        for key,value in exact['characteristics'].items())
            assert checks['mrk_probability_difference'] < 1e-6, checks
            assert checks['mrk_characteristic_difference'] < 1e-4, checks
            variation = client.call('run_variation', {'path': path, 'points': 96,
                          'vary': [{'element': 'Lam', 'start': .05, 'step': .01}]})
            assert len(variation['points']) == 96
            series = []
            for k, point in enumerate(variation['points']):
                actual = metrics(index, point['characteristics'])
                expected = module.analyze(.05 + .01*k)['metrics']
                assert max(abs(actual[key]-expected[key]) for key,_ in PLOT_KEYS) < 1e-4
                series.append(actual)
            curves.append(series)
            client.call('set_graphs', {'path': path, 'graphs': [
                {'name': name, 'marker': 'none', 'line_width': 2}
                for name in ('Loss', 'Throughput', 'Busy', 'Queue', 'Wait', 'Sojourn')]})
            client.call('export_report', {'path': path, 'output': str(OUT / f'system{index}.html'),
                         'options': {'matrix': True, 'probabilities': True, 'characteristics': True,
                                     'varied_probabilities': True, 'varied_characteristics': True}})
            reloaded = client.call('get_model', {'path': path})
            assert reloaded['states'] == len(q)
            assert reloaded['variation_computed_points'] == 96
            assert len(reloaded['graphs']) == 6
            save_json(f'system{index}_inline.json', exact)
            save_json(f'system{index}_native.json', reloaded)
            save_json(f'system{index}_variation.json', variation)
            data['verification'][f'system{index}'] = checks
            print(f'SYSTEM {index}: {len(pi)} states; loss={mm["total_loss"]:.12f}; checks={checks}')
    finally:
        client.close()
        save_json('mcp_transcript.json', client.requests)
    plt.rcParams.update({'font.family': 'DejaVu Sans', 'font.size': 10, 'svg.fonttype': 'none'})
    grid = np.linspace(.05, 1, 96)
    for page in range(3):
        fig, axes = plt.subplots(2, 1, figsize=(7.2, 7.6), layout='constrained')
        for ax, (key, title) in zip(axes, PLOT_KEYS[2*page:2*page+2]):
            for i, color in ((0, '#205a8e'), (1, '#a24a30')):
                ax.plot(grid, [point[key] for point in curves[i]], label=f'СИСТЕМА_{i+1}', color=color, lw=1.8)
                ax.plot(.5, data['systems'][i]['metrics'][key], 'o', color=color, ms=5)
            ax.axvline(.5, color='gray', ls=':', lw=.9)
            ax.set(xlabel='Интенсивность входного потока λ, 1/с', ylabel=title, xlim=(.05, 1))
            ax.grid(alpha=.22)
            ax.legend(fontsize=9)
        fig.savefig(OUT / f'comparison{page+1}.svg')
        plt.close(fig)
    with (OUT / 'sensitivity.csv').open('w', encoding='utf-8-sig', newline='') as file:
        writer = csv.writer(file)
        writer.writerow(['lambda'] + [f'system{i}_{key}' for key,_ in PLOT_KEYS for i in (1,2)])
        for k, lam in enumerate(grid):
            writer.writerow([float(lam)] + [curves[i][k][key] for key,_ in PLOT_KEYS for i in (0,1)])
    save_json('data.json', data)
    print(f'MacMark {client.server["version"]}: 2 native models, 192 variation points, 2 HTML reports')


if __name__ == '__main__':
    main()
