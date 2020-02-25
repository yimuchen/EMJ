import math, os

class emjHelper(object):
  def __init__(self):
    with open(
        os.path.join(os.path.expandvars('$CMSSW_BASE'),
                     'src/EMJ/Production/data/ZprimeCrossSection.txt'),
        'r') as xfile:
      self.xsecs = {
          int(xline.split()[0]): float(xline.split()[1])
          for xline in xfile
      }
    self.alphaName = ''
    # parameters for lambda/alpha calculations
    self.n_c = 2
    self.n_f = 2
    self.b0 = 11.0 / 6.0 * self.n_c - 2.0 / 6.0 * self.n_f

  def setAlpha(self, alpha):
    self.alphaName = alpha
    # 'empirical' formula
    lambda_peak = 3.2 * math.pow(self.mDark, 0.8)
    if self.alphaName == 'peak':
      self.alpha = self.calcAlpha(lambda_peak)
    elif self.alphaName == 'high':
      self.alpha = 1.5 * self.calcAlpha(lambda_peak)
    elif self.alphaName == 'low':
      self.alpha = 0.5 * self.calcAlpha(lambda_peak)
    else:
      raise ValueError('unknown alpha request: ' + alpha)

  # calculation of lambda to give desired alpha
  # see 1707.05326 fig2 for the equation: alpha = pi/(b * log(1 TeV / lambda)), b = 11/6*n_c - 2/6*n_f
  # n_c = HiddenValley:Ngauge, n_f = HiddenValley:nFlav
  # see also TimeShower.cc in Pythia8, PDG chapter 9 (Quantum chromodynamics), etc.

  def calcAlpha(self, lambdaHV):
    return math.pi / (self.b0 * math.log(1000 / lambdaHV))

  def calcLambda(self, alpha):
    return 1000 * math.exp(-math.pi / (self.b0 * alpha))

  # has to be 'lambdaHV' because 'lambda' is a keyword
  def setModel(self, mX, mDPi, tauDPi):
    self.mX = mX
    self.mDPi = mDPi
    self.tauDPi   = tauDPi

    self.xsec = self.getPythiaXsec(mX)
    return

  def getOutName(self, signal=True,events=0, outpre='outpre', part=None):
    _outname = outpre
    if signal:
      _outname += '_mX-{:g}'.format(self.mX)
      _outname += '_mDPi-{:g}'.format(self.mDPi)
      _outname += '_tauDPi-{:g}'.format(self.tauDPi)
    if events > 0: _outname += '_n-{:g}'.format(events)
    if part is not None:
      _outname += '_part-{:g}'.format(part)
    return _outname

  # allow access to all xsecs
  def getPythiaXsec(self, mass):
    xsec = 1.0
    # a function of mZprime
    if mass in self.xsecs: xsec = self.xsecs[mass]
    return xsec

  def getPythiaSettings(self):
    lines = [
        'Init:showChangedSettings = on',  # list changed settings
        'Init:showChangedParticleData = on',  # list changed particle data
        'Next:showScaleAndVertex = on',
        'Next:numberCount = 100',  # print message every n events
        'Next:numberShowInfo = 10',  # print event information n times
        'Next:numberShowProcess = 10',  # print process record n times
        'Next:numberShowEvent = 10',  # print event record n times
        'ParticleDecays:xyMax = 30000',  # in mm/c
        'ParticleDecays:zMax = 30000',  # in mm/c
        'ParticleDecays:limitCylinder = on',  # yes
        'HiddenValley:gg2DvDvbar = on',  # gg fusion
        'HiddenValley:qqbar2DvDvbar = on',  # qqbar fusion
        'HiddenValley:alphaOrder = 1',  # Let it run
        'HiddenValley:Ngauge = 3',  # Number of dark QCD colours
        'HiddenValley:nFlav = 7',  # flavours used for the running
        'HiddenValley:FSR = on',
        'HiddenValley:fragment = on',
        'HiddenValley:spinFv = 0',  # Spin of bi-fundamental res.
        # Mass of bi-fundamental resonance
        '4900001:m0 = {0}'.format(self.mX),
        # Width of bi-fundamental resonance
        '4900001:mWidth = 10',
        '4900002:m0 = 50000',
        '4900003:m0 = 50000',
        '4900004:m0 = 50000',
        '4900005:m0 = 50000',
        '4900006:m0 = 50000',
        'HiddenValley:Lambda={0}'.format(self.mDPi * 2 ),
        'HiddenValley:pTminFSR = 11.0',
        # dark quark mass = LambdaHV
        '4900101:m0 = {0}'.format(self.mDPi * 2 ),
        # dark scalar (pion) mass
        '4900111:m0 = {0}'.format(self.mDPi),
        '4900211:m0 = {0}'.format(self.mDPi),
        # dark scalar (pion) lifetime (in mm)
        '4900111:tau0 = {0}'.format(self.tauDPi),
        '4900211:tau0 = {0}'.format(self.tauDPi),
        # dark vector (rho) mass
        '4900113:m0 = {0}'.format(self.mDPi * 4 ) ,
        '4900213:m0 = {0}'.format(self.mDPi * 4 ) ,
        # Resonance decays to dark quark + flavoured SM
        # dark pion decay to down quarks
        '4900111:0:all      =  1 1.000  91        1       -1',
        # dark vector to dark pions 99.9%
        '4900113:0:all      =  1 0.999  91  4900111  4900111',
        # dark vector to down quarks 0.1%
        '4900113:addchannel =  1 0.001  91        1       -1',
        # dark pion decay to down quarks
        '4900211:oneChannel =  1 1.000  91        1       -1',
         # dark vector to dark pions 99.9%
        '4900213:oneChannel =  1 0.999  91  4900211  4900211',
        # dark vector to down quarks 0.1%
        '4900213:addchannel =  1 0.001  91        1       -1',
    ]

    return lines
