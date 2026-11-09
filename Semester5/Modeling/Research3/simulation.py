"""Независимая событийная реализация G/G/k/E, FIFO, без прерываний."""
from heapq import heappop, heappush
import math
import numpy as np


def h2_parameters(mean, cv):
    if mean <= 0 or cv <= 1:
        raise ValueError('Для H2 нужны mean > 0 и cv > 1')
    p = (1 + math.sqrt((cv * cv - 1) / (cv * cv + 1))) / 2
    return p, mean / (2 * p), mean / (2 * (1 - p))


def sample(rng, law, mean, size, cv=2, trace=None):
    if mean <= 0 or size < 1:
        raise ValueError('Среднее и размер выборки должны быть положительными')
    if law == 'M':
        return rng.exponential(mean, size)
    if law == 'E2':
        return rng.gamma(2, mean / 2, size)
    if law == 'H2':
        p, fast, slow = h2_parameters(mean, cv)
        u = rng.random((size, 2))
        return -np.log1p(-u[:, 1]) * np.where(u[:, 0] < p, fast, slow)
    if law == 'U':
        return rng.uniform(mean / 2, 1.5 * mean, size)
    if law == 'D':
        return np.full(size, mean)
    if law == 'T':
        if trace is None or len(trace) == 0 or np.any(np.asarray(trace) <= 0):
            raise ValueError('Нужна непустая положительная трасса')
        # Случайный циклический сдвиг сохраняет порядок и долгосрочное среднее.
        offset = int(rng.integers(len(trace)))
        return np.resize(np.roll(trace, -offset), size) * mean / np.mean(trace)
    raise ValueError(f'Неизвестный закон: {law}')


def simulate_intervals(intervals, services, k, capacity, warmup=0, prefer=None):
    """Приёмная когорта досчитывается целиком; интегралы только в окне наблюдения.

    capacity=None означает бесконечный накопитель. Время измеряется в секундах.
    При совпадении событий завершение предшествует поступлению.
    """
    intervals, services = np.asarray(intervals), np.asarray(services)
    if (k < 1 or (capacity is not None and capacity < 0) or
            len(intervals) != len(services) or not 0 <= warmup < len(intervals) or
            np.any(intervals <= 0) or np.any(services <= 0) or
            not np.all(np.isfinite(intervals)) or not np.all(np.isfinite(services))):
        raise ValueError('Некорректные параметры модели или длительности')
    arrivals = np.cumsum(intervals)
    available = [0.] * k
    departures = []
    records = []
    losses = 0
    for i, (now, duration) in enumerate(zip(arrivals, services)):
        while departures and departures[0] <= now:
            heappop(departures)
        if capacity is not None and len(departures) >= k + capacity:
            losses += i >= warmup
            continue
        free = [s for s in range(k) if available[s] <= now]
        if len(free) == k == 2 and prefer is not None:
            server = 0 if prefer[i] < .65 else 1
        else:
            server = free[0] if free else min(range(k), key=available.__getitem__)
        start = max(now, available[server])
        finish = start + duration
        available[server] = finish
        heappush(departures, finish)
        records.append((i, now, start, finish, server))
    r = np.asarray(records).reshape(-1, 5)
    begin = arrivals[warmup - 1] if warmup else 0.
    end = arrivals[-1]
    elapsed = end - begin
    cohort = r[r[:, 0] >= warmup]
    wait = cohort[:, 2] - cohort[:, 1]
    sojourn = cohort[:, 3] - cohort[:, 1]
    queue_area = np.maximum(0, np.minimum(r[:, 2], end) - np.maximum(r[:, 1], begin)).sum()
    busy_area = np.maximum(0, np.minimum(r[:, 3], end) - np.maximum(r[:, 2], begin))
    busy = [float(busy_area[r[:, 4] == s].sum() / elapsed) for s in range(k)]
    n = len(intervals) - warmup
    result = dict(n=n, accepted=len(cohort), lost=int(losses), loss=losses/n,
                  wait=float(wait.mean()), sojourn=float(sojourn.mean()),
                  wait_std=float(wait.std(ddof=1)), sojourn_std=float(sojourn.std(ddof=1)),
                  wait_cv=float(wait.std(ddof=1)/wait.mean()) if wait.mean() else 0.,
                  sojourn_cv=float(sojourn.std(ddof=1)/sojourn.mean()),
                  queue=float(queue_area/elapsed), utilization=float(sum(busy)/k),
                  number=float(queue_area/elapsed+sum(busy)),
                  throughput=float(np.sum((r[:, 3] > begin) & (r[:, 3] <= end))/elapsed),
                  elapsed=float(elapsed), busy=busy,
                  zero_wait=float(np.mean(wait < 1e-10)))
    assert result['accepted'] + result['lost'] == n
    assert 0 <= result['utilization'] <= 1 + 1e-10
    if capacity is not None:
        assert result['queue'] <= capacity + 1e-9
    return result, wait, sojourn


def simulate(config, n=100_000, warmup=20_000, seed=2026, trace=None):
    # Независимые потоки: смена закона поступления не сдвигает обслуживание.
    streams = np.random.SeedSequence(seed).spawn(3)
    a = sample(np.random.default_rng(streams[0]), config['arrival'], config['a'],
               n+warmup, cv=config.get('arrival_cv', 2), trace=trace)
    b = sample(np.random.default_rng(streams[1]), config['service'], config['b'], n+warmup)
    prefer = np.random.default_rng(streams[2]).random(n+warmup)
    return simulate_intervals(a, b, config['k'], config['capacity'], warmup, prefer)


def mmke(k, capacity, a, b):
    """Точное стационарное решение M/M/k с k+E местами всего."""
    weights = [1.]
    for i in range(1, k+capacity+1):
        weights.append(weights[-1] * b/a / min(i, k))
    pi = np.asarray(weights)/sum(weights)
    states = np.arange(len(pi))
    loss = float(pi[-1])
    throughput = (1-loss)/a
    queue = float(pi @ np.maximum(states-k, 0))
    utilization = float(pi @ np.minimum(states, k)/k)
    return dict(loss=loss, wait=queue/throughput, sojourn=queue/throughput+b,
                queue=queue, utilization=utilization, throughput=throughput,
                number=float(pi @ states))


def self_check():
    # Два ожидания/отказа, включая одновременное освобождение и поступление.
    r, w, t = simulate_intervals([1, 1, 1, 1], [3, 3, 3, 3], 1, 1)
    assert r['accepted'] == 3 and r['lost'] == 1
    np.testing.assert_allclose(w, [0, 2, 3])
    np.testing.assert_allclose(t, [3, 5, 6])
    assert abs(r['queue'] - .5) < 1e-12
    r, w, _ = simulate_intervals([2]*20, [1]*20, 1, 0, warmup=3)
    assert r['lost'] == 0 and np.all(w == 0)
    exact = mmke(2, 7, 2, 10)
    assert abs(exact['loss']-.6001101206872341) < 1e-12
    p, f, s = h2_parameters(10, 2)
    assert abs(p*f+(1-p)*s-10) < 1e-12
    assert abs((2*(p*f*f+(1-p)*s*s)-100)/100-4) < 1e-12
    print('Проверки FIFO, отказов, границ окна, M/M/2/7 и H2 пройдены.')


if __name__ == '__main__':
    self_check()
