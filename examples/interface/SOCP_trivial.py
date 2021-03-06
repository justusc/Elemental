# The simplest example of solving an SOCP with Elemental.
# Contributed by Steven Diamond.
import El
El.Initialize()
orders = El.DistMultiVec(El.iTag)
orders.Resize(1, 1)
orders.Set(0, 0, 1)
labels = El.DistMultiVec(El.iTag)
labels.Resize(1, 1)
labels.Set(0, 0, 0)
firstInd = El.DistMultiVec(El.iTag)
firstInd.Resize(1, 1)
firstInd.Set(0, 0, 0)

c = El.DistMultiVec()
c.Resize(1, 1)
c.Set(0, 0, -1.)

h = El.DistMultiVec()
h.Resize(1, 1)
h.Set(0, 0, 1.)

b = El.DistMultiVec()
b.Resize(0, 1)

A = El.DistSparseMatrix()
A.Resize(0, 1)
A.ProcessQueues()

G = El.DistSparseMatrix()
G.Resize(1, 1)
G.Reserve(1)
G.QueueUpdate(0, 0, 1.)
G.ProcessLocalQueues()

x = El.DistMultiVec()
y = El.DistMultiVec()
z = El.DistMultiVec()
s = El.DistMultiVec()
ctrl = El.SOCPAffineCtrl_d()
ctrl.mehrotraCtrl.qsdCtrl.progress = True
ctrl.mehrotraCtrl.progress = True
ctrl.mehrotraCtrl.outerEquil = True
ctrl.mehrotraCtrl.time = True
El.SOCPAffine(A,G,b,c,h,orders,firstInd,labels,x,y,z,s,ctrl)
El.Finalize()
