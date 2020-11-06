import math, os, numpy

# Physical constants
hbarc = 1.97e-13  # in GeV mm


class emjHelper(object):
  def __init__(self):
    # Aligned mixing elements
    self.s12 = 0
    self.s13 = 0
    self.s23 = 0
    self.kappa0 = 1
    self.kap1 = 0
    self.kap2 = 0
    self.BuildMatrix()

  def setModel(self, mMed, mDark, kappa, mode='aligned', type="down"):
    self.mMed = mMed
    self.mDark = mDark
    self.kappa0 = kappa
    self.mode = mode
    self.type = type

    # Checking the alignment mode
    if self.mode == 'aligned':
      self.s12 = 0
      self.s13 = 0
      self.s23 = 0
      self.kap1 = 0
      self.kap2 = 0
    elif self.mode == 'unflavored':
      pass
    else:
      raise Exception('Mode not recognized!')
    self.BuildMatrix()

    # Checking the coupling type
    if self.type == 'down':
      self.sm_id = [1, 3, 5]
      self.sm_mass = [0.0048, 0.093, 4.18]
    elif self.type == 'up':
      self.sm_id = [2, 4, 6]
      self.sm_mass = [0.0023, 1.275, 173.21]
    else:
      raise Exception('Type not recognized')
    return

  def BuildMatrix(self):
    # Generatin the mixing matrix
    self.U12 = numpy.matrix([[numpy.sqrt(1 - self.s12**2), self.s12, 0],
                             [-self.s12,
                              numpy.sqrt(1 - self.s12**2), 0], [0, 0, 1]])
    self.U13 = numpy.matrix([[numpy.sqrt(1 - self.s13**2), 0, self.s13],
                             [0, 1, 0],
                             [-self.s13, 0,
                              numpy.sqrt(1 - self.s13**2)]])
    self.U23 = numpy.matrix([[1, 0, 0],
                             [0, numpy.sqrt(1 - self.s23**2), self.s23],
                             [0, -self.s23,
                              numpy.sqrt(1 - self.s23**2)]])
    self.D = numpy.matrix([[self.kappa0 * (1 + self.kap1), 0, 0],
                           [0, self.kappa0 * (1 + self.kap2), 0],
                           [0, 0, self.kappa0 * (1 - self.kap1 - self.kap2)]])
    self.kappa = self.U12 * self.U13 * self.U23 * self.D
    self.kNorm = float(numpy.square(self.kappa).sum())

  def getOutName(self, signal=True, events=0, outpre='outpre', part=None):
    _outname = outpre
    if signal:
      _outname += '_mMed-{:g}'.format(self.mMed)
      _outname += '_mDark-{:g}'.format(self.mDark)
      _outname += '_{}-{:g}'.format(
          'kappa' if self.mode != 'unflavored' else 'ctau', self.kappa0).replace(
              '.', 'p')
      _outname += '_{}-{}'.format(self.mode, self.type)
    if events > 0: _outname += '_n-{:g}'.format(events)
    if part is not None:
      _outname += '_part-{:g}'.format(part)
    return _outname

  def gamma_pre(self):
    form = self.mDark
    return (3 * self.mDark * form**2) / (32 * numpy.pi * self.mMed**4)

  def mass_factor(self, m1, m2):
    if (m1 + m2) * 1.15 > self.mDark:
      return 0
    else:
      ans = (m1 * m1 + m2 * m2)
      ans = ans * numpy.sqrt(1.0 - (((m1 + m2) / self.mDark)**2))
      ans = ans * numpy.sqrt(1.0 - (((m1 - m2) / self.mDark)**2))
      return ans

  def calc_gamma(self, dark1, dark2, sm1, sm2):
    k11 = self.kappa.item((dark1, sm1))
    k12 = self.kappa.item((dark1, sm2))
    k22 = self.kappa.item((dark2, sm2))
    k21 = self.kappa.item((dark2, sm1))

    m1 = self.sm_mass[sm1]
    m2 = self.sm_mass[sm2]
    if dark1 == dark2:
      ans = (k11 * k22)**2 * self.gamma_pre()
      ans = ans * self.mass_factor(m1, m2) / 2
      return ans
    elif sm1 == sm2:
      ans = (k11 * k22)**2 * self.gamma_pre()
      ans = ans * self.mass_factor(m1, m2)
      return ans
    else:
      ans = (k11 * k22 + k12 * k21)**2 * self.gamma_pre()
      ans = ans * self.mass_factor(m1, m2)
      return ans

  def getPythiaSettings(self):

    lines = [
        'Init:showChangedSettings = on',  # list changed settings
        'Init:showChangedParticleData = on',  # list changed particle data
        'Next:showScaleAndVertex = on',
        'Next:numberCount = 100',  # print message every n events
        'Next:numberShowInfo = 1',  # print event information n times
        'Next:numberShowProcess = 1',  # print process record n times
        'Next:numberShowEvent = 1',  # print event record n times
        'ParticleDecays:xyMax = 30000',  # in mm/c
        'ParticleDecays:zMax = 30000',  # in mm/c
        'ParticleDecays:limitCylinder = on',  # yes
        'HiddenValley:gg2DvDvbar = on',  # gg fusion
        'HiddenValley:qqbar2DvDvbar = on',  # qqbar fusion
        'HiddenValley:alphaOrder = 1',  # Let it run
        'HiddenValley:Ngauge = 3',  # Number of dark QCD colours
        'HiddenValley:nFlav = {}'.format(7 if self.mode == 'unflavored' else 3
                                         ),  # flavours used for the running
        'HiddenValley:FSR = on', 'HiddenValley:fragment = on',
        'HiddenValley:altHadronSpecies = {0}'.format(
            'off' if self.mode ==
            'unflavored' else 'on'),  # implements arXiv:1803.08080
        'HiddenValley:spinFv = 0',  # Spin of bi-fundamental res.
        # define some missing antiparticles
        'HiddenValley:Lambda={0}'.format(self.mDark * 2),
        'HiddenValley:pTminFSR = 11.0', '4900101:m0={0}'.format(self.mDark * 2),
    ]

    lines.extend(self.MakeRes())
    lines.extend(self.MakeDecay())
    return lines

  def MakeRes(self):
    lines = [  # Mass of bi-fundamental resonance
        '4900001:m0 = {0}'.format(self.mMed),
        # Width of bi-fundamental resonance
        '4900001:mWidth = 10',
        # Other resonance masses are set to unreachable limits
        '4900002:m0 = 50000', '4900003:m0 = 50000', '4900004:m0 = 50000',
        '4900005:m0 = 50000', '4900006:m0 = 50000'
    ]

    if self.mode == 'unflavored':
      pass
    else:
      sm_list = [1, 3, 5] if self.type == 'down' else [2, 4, 6]
      dark_list = [4900101, 4900102, 4900103]
      for sm_idx, sm_quark in enumerate(sm_list):
        for d_idx, dark_quark in enumerate(dark_list):
          lines.append('4900001:{0}Channel = 1 {3} 103 {1} {2}'.format(
              'one' if sm_idx == d_idx and sm_idx == 0 else 'add', sm_quark,
              dark_quark,
              self.kappa.item((d_idx, sm_idx))**2 / self.kNorm))
    return lines

  def MakeDecay(self):
    def gamma(dark_comp1, dark_comp2):
      return sum([
          self.calc_gamma(dark_comp1, dark_comp2, i, j)
          for i in range(0, 3)
          for j in range(i, 3)
      ])

    def extend_decay(dark_meson, dark_comp1, dark_comp2):
      ans = ['{}:m0 = {}'.format(dark_meson, self.mDark)]
      gamma_sum = gamma(dark_comp1, dark_comp2)
      if gamma_sum > 0:
        ans.append('{}:tau0 = {}'.format(dark_meson, hbarc / gamma_sum))
        ans.extend([
            '{0}:{1}Channel = 1 {4} 91 {2} -{3}'.format(
                dark_meson, 'one' if i == 0 and j == 0 else 'add', self.sm_id[i],
                self.sm_id[j],
                self.calc_gamma(dark_comp1, dark_comp2, i, j) / gamma_sum)
            for i in range(3)
            for j in range(i, 3)
        ])
      return ans

    if self.mode == 'unflavored':  # Special case for unflavored decay
      sm_id = 1 if self.type == 'down' else 2
      return [
          '4900111:m0 = {0}'.format(self.mDark), '4900211:m0 = {0}'.format(
              self.mDark), '4900111:tau0 = {0}'.format(
                  self.kappa0), '4900211:tau0 = {0}'.format(
                      self.kappa0), '4900113:m0 = {0}'.format(
                          4 * self.mDark), '4900213:m0 = {0}'.format(
                              4 * self.mDark),
          '4900111:0:all      =  1 1.000  91      {0}     -{0}'.format(sm_id),
          '4900113:0:all      =  1 0.999  91  4900111  4900111',
          '4900113:addchannel =  1 0.001  91      {0}     -{0}'.format(sm_id),
          '4900211:oneChannel =  1 1.000  91      {0}     -{0}'.format(sm_id),
          '4900213:oneChannel =  1 0.999  91  4900211  4900211',
          '4900213:addchannel =  1 0.001  91      {0}     -{0}'.format(sm_id),
      ]

    ## PDG ID should match with hidden valley definition
    ## https://github.com/kpedro88/pythia8/blob/emg/230/src/HiddenValleyFragmentation.cc

    # Defining some missing particles
    meson_decay = [
        '4900111:antiName = pivDiagbar', '4900113:antiName = rhovDiagbar',
    ]
    meson_decay.extend(extend_decay(4900113, 0, 1))
    meson_decay.extend(extend_decay(4900211, 0, 2))
    meson_decay.extend(extend_decay(4900213, 1, 2))

    ## Neutral PI is the same flavour one
    neutral_pi_gamma = [gamma(i, i) for i in range(3)]
    pi_comp = neutral_pi_gamma.index(max(neutral_pi_gamma))
    meson_decay.extend(extend_decay(4900111, pi_comp, pi_comp))

    return meson_decay


if __name__ == "__main__":
  import sys, argparse, re
  helper = emjHelper()

  parser = argparse.ArgumentParser(
      'Calculation debugging for Emerging jets pythia settings ')
  parser.add_argument('--mMed',
                      default=1000,
                      type=float,
                      help='Dark mediator mass [GeV]')
  parser.add_argument('--kappa',
                      default=1,
                      type=float,
                      help=('Kappa0 (SM-Dark sector compling constant) if '
                            'mode==aligned, dark pion lifetime[mm] if '
                            'mode == unflavored '))
  parser.add_argument('--mDark',
                      default=10,
                      type=float,
                      help='Dark meson mass [GeV]')
  parser.add_argument('--type',
                      default='down',
                      type=str,
                      choices=['down', 'up'],
                      help='Alignment to SM up/down type SM quarks')
  parser.add_argument('--mode',
                      default='aligned',
                      type=str,
                      choices=['aligned', 'unflavored'],
                      help='Mixing scenarios to simulate')
  if (len(sys.argv) < 2):
    print "Use either 'dumptime' or 'dumpcard' sub commands"
    sys.exit(0)

  args = parser.parse_args(sys.argv[2:])

  if (sys.argv[1] == 'dumptime'):
    for mDark in numpy.linspace(1.6, 100, 1000, endpoint=True):
      helper.setModel(args.mMed, mDark, args.kappa, args.mode, args.type)
      tau = [
          x for x in helper.getPythiaSettings()
          if re.match(r'^4900[12]1[13]:tau0', x)
      ]

      def get_time(pdg_id):
        m = [
            float(re.sub('=', '', re.sub('\d*:tau0', '', t)))
            for t in tau
            if re.match('^{}:tau0'.format(pdg_id), t)
        ]
        if len(m):
          return m[0]
        else:
          return 1e12

      print '{:10g} {:16g} {:16g} {:16g} {:16g}'.format(mDark, get_time(4900111),
                                                        get_time(4900113),
                                                        get_time(4900211),
                                                        get_time(4900213))
  elif (sys.argv[1] == 'dumpcard'):
    helper.setModel(args.mMed, args.mDark, args.kappa, args.mode, args.type)
    for line in helper.getPythiaSettings():
      print line
