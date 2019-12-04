#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import nlo.repo

repo = nlo.repo.repo_at_script(__file__)

repo.library("zoo", ".")

repo.process()
