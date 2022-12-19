import os, unittest

import gi, gi.overrides
gi.overrides.__path__ = os.environ["DEE_TEST_PYGOBJECT_OVERRIDEDIR"]
print "Running Python tests with overrides path '%s'" % gi.overrides.__path__
from gi.repository import Dee

class TreeIndexTest (unittest.TestCase):

	model = None
	analyzer = None
	index = None	
	
	def testEmpty (self):
		self.model = Dee.SequenceModel.new ()
		self.model.set_schema ("i", "s")
		self.analyzer = Dee.TextAnalyzer.new ()
		
		def readString (model, iter, data):
			return model[iter][1]
		
		self.index = Dee.TreeIndex.new (self.model, self.analyzer, readString, None)
		
		row = self.model.append (1, "one")
		result = self.index.lookup_one ("one")
		
		self.assertEquals (row, result)
		self.assertEquals (self.model[row], self.model[result])

if __name__ == '__main__':
	unittest.main()
