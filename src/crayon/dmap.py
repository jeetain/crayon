#
# dmap.py
# pure python implementation of basic diffusion map calculation
#
# Copyright (c) 2018 Wesley Reinhart.
# This file is part of the crayon project, released under the Modified BSD License.

from __future__ import print_function
import pickle

import numpy as np

from crayon import parallel
from crayon import util

class DMap:
    R""" container for computing diffusion maps from Ensembles of Snapshots """
    def __init__(self, num_evec=4):
        self.alpha = 1.0
        self.num_evec = num_evec
        self.epsilon = None
        self.evals = None
        self.evecs = None
        self.evecs_ny = None
        self.coords = None
    def compute(self,dists,epsilon):
        R"""Compute diffusion map embedding from square distance matrix."""
        #
        self.epsilon = epsilon
        self.dists = dists
        #
        inv_neg_2eps = -1./(2.*self.epsilon)
        kernel = np.exp(dists**2 * inv_neg_2eps)
        rsum = np.sum(kernel, axis=1)
        kernel = np.matmul( np.diag(1./rsum), kernel )
        #
        evals, evecs = np.linalg.eig(kernel)
        evals = np.real(evals)
        evecs = np.real(evecs)
        idx = np.argsort(evals)[::-1]
        #
        self.evals = evals[idx][1:(1+self.num_evec)]
        self.evecs = evecs[:,idx][:,1:(1+self.num_evec)]
    def embed(self,dists):
        R"""Embed new samples using Nystrom approximation."""
        d = np.exp(-dists**2/(2.*self.epsilon))
        inv_rsum = 1./np.sum(d,axis=1)
        d *= inv_rsum.reshape(-1,1)
        loc = np.matmul( d, self.eigenvec ) / self.eigenval
        return loc
    def build(self,dists,landmarks=None,
              valid_cols=None,
              valid_rows=None):
        R""" compute the diffusion map from pre-computed distances

        Args:
            dists (array): distances between signatures pairwise or against landmarks
            landmarks (array,optional): landmark indices (assume full pairwise by default)
            valid_cols (array,optional): columns to include in the DMap (default all columns)
            valid_rows (array,optional): rows to include in the DMap (default all rows)
        """
        if valid_rows is None:
            valid_rows = np.arange(dists.shape[0])
        if valid_cols is None:
            valid_cols = np.arange(dists.shape[1])
        alpha_dists = np.power(dists,self.alpha)
        if landmarks is None:
            D = alpha_dists
            D = D[valid_rows,:]
            D = D[:,valid_cols]
        else:
            D = alpha_dists[landmarks,:]
            D = D[:,valid_cols]
            L = alpha_dists[valid_rows,:]
            L = L[:,valid_cols]
        # default to median of distances
        if self.epsilon is None:
            self.epsilon = np.median(alpha_dists)
        # compute landmark manifold
        self.compute(D, self.epsilon)
        # embed remaining graphs using landmarks
        self.evecs_ny = self.embed(L)
        # # compute landmark manifold (old way!)
        # self._cpp.set_dists(D)
        # self._cpp.set_num_evec(self.num_evec)
        # if self.epsilon is None:
        #     self.epsilon = np.median(alpha_dists)
        # self._cpp.set_epsilon(self.epsilon)
        # self._cpp.compute()
        # self.evals = np.asarray(self._cpp.get_eval())
        # self.evecs = np.asarray(self._cpp.get_evec())
        # # embed remaining graphs using landmarks (old way!)
        # if landmarks is None:
        #     self.evecs_ny = None
        # else:
        #     self.evecs_ny = np.asarray(PyDMap.nystrom(self._cpp,L))
        # backfill for invalid graphs
        evecs = np.zeros((dists.shape[1],self.num_evec))*np.nan
        evecs[valid_cols,:] = np.array(self.evecs)
        self.evecs = evecs
        evecs_ny = np.zeros((dists.shape[0],self.num_evec))*np.nan
        evecs_ny[valid_rows,:] = np.array(self.evecs_ny)
        self.evecs_ny = evecs_ny
        # construct uniformly coordinates for mapping to RGB space
        if self.evecs_ny is None:
            R = self.evecs
        else:
            R = self.evecs_ny
        self.coords = R
        self.color_coords = util.rankTransform(R)
        # first eigenvector is always trivial
        self.color_coords[:,0] = 0.5
        # catch nan rows
        nan_idx = np.argwhere(np.isnan(self.color_coords[:,-1])).flatten()
        self.color_coords[nan_idx,:] = 1.
