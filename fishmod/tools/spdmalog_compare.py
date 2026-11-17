import sys,collections
def load(p):
    tasks={};cur=None
    for l in open(p):
        f=l.split()
        if f[0]=='task': cur=int(f[1]); tasks[cur]=[]
        elif cur is not None: tasks[cur].append((f[0],f[1],int(f[2],16),f[3],f[4]))
    return tasks
a=load(sys.argv[1]); b=load(sys.argv[2])
sig=collections.Counter(); per={}
for t in sorted(a):
    if t not in b or len(a[t])!=len(b[t]): print('task',t,'counts differ'); continue
    d=[(i,a[t][i],b[t][i]) for i in range(len(a[t])) if a[t][i][4]!=b[t][i][4]]
    per[t]=d
    kinds=collections.Counter((x[0],x[1],x[3]) for i,x,y in d)
    print(t,' '.join('%s%s/%s:%d'%(k[0],k[1],k[2],v) for k,v in sorted(kinds.items()) if k[1]!='2f0'))
