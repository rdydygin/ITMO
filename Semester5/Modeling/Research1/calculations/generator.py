"""Воспроизводимый расчет УИР 1. Запуск: python generator.py [variant.md].
Зависимости: numpy, scipy. Без аргумента используются приложенные исходные данные.
"""
from pathlib import Path
import sys
import numpy as np
from scipy.stats import t


def characteristics(a):
    n=len(a); mean=a.mean(); var=a.var(ddof=1); sd=np.sqrt(var)
    return np.array([mean, *[t.ppf((1+g)/2,n-1)*sd/np.sqrt(n) for g in (.9,.95,.99)], var,sd,sd/mean])


def generate(x, seed=2026):
    mean=x.mean(); c2=x.var(ddof=1)/mean**2
    if c2 <= 1: raise ValueError('Для H2 требуется коэффициент вариации > 1')
    p=(1+np.sqrt((c2-1)/(c2+1)))/2
    rates=(2*p/mean,2*(1-p)/mean)
    rng=np.random.default_rng(seed)
    u=rng.random(len(x)); v=rng.random(len(x))
    y=-np.log1p(-v)/np.where(u<p,rates[0],rates[1])
    return y,p,rates


if __name__=='__main__':
    source=Path(sys.argv[1]) if len(sys.argv)>1 else Path(__file__).with_name('source_sequence.csv')
    x=np.loadtxt(source,delimiter=',' if source.suffix=='.csv' else None)
    y,p,rates=generate(x)
    out=Path(__file__).parent
    np.savetxt(out/'sequences.csv',np.column_stack((np.arange(1,len(x)+1),x,y)),delimiter=',',header='index,original,generated',comments='',fmt=['%d','%.3f','%.12f'])
    labels=['mean','halfwidth_0.90','halfwidth_0.95','halfwidth_0.99','variance','std','cv']
    rows=[]
    ref=characteristics(x)
    for n in [10,20,50,100,200,300]:
        a,b=characteristics(x[:n]),characteristics(y[:n])
        for j,label in enumerate(labels): rows.append([n,label,a[j],100*(a[j]/ref[j]-1),b[j],100*(b[j]/ref[j]-1),100*(b[j]/a[j]-1)])
    import csv
    with (out/'characteristics.csv').open('w',newline='',encoding='utf-8') as f:
        w=csv.writer(f);w.writerow(['n','metric','original','original_deviation_pct','generated','generated_deviation_pct','generated_vs_same_n_pct']);w.writerows(rows)
    print(f'p={p:.12f}; rates={rates}; correlation={np.corrcoef(x,y)[0,1]:.9f}')
    for k in range(1,16): print(k,np.corrcoef(x[:-k],x[k:])[0,1],np.corrcoef(y[:-k],y[k:])[0,1])
