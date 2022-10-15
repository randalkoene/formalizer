# Copyright 2022 Randal A. Koene
# License TBD

"""
Import this module to provide nutrition data. For example, see
how this is used by `daywiz.py`.

Versioning is based on https://semver.org/. See coreversion.hpp for more.
"""

WEIGHTLOSS_TARGET_CALORIES=1750
UNKNOWN_DAY_CALORIES_ASSUMED=2300
MIN_CALORIES_THRESHOLD=1000

# ====================== Nutrition information:

nutrition = {
	'coffee': [ 5, 'cup', ],
	'almond milk': [ 90, 'cup', ],
	'soylent': [ 400, 'bottle', ],
	'protein shake': [ 180, 'bottle', ],
	'yogurt': [ 140, '4oz', ],
	'dried apricots': [ 18, 'apricot', ],
	'raisins': [ 6, 'raisin', ],
	'dried figs': [ 36, 'fig', ],
	'celery': [ 7, 'stalk', ],
	'tomato grape': [ 1, 'grape', ],
	'orange': [ 45, 'orange', ],
	'mandarin': [ 47, 'mandarin', ],
	'blueberries': [ 85, 'cup', ],
	'avocado': [ 322, 'avocado', ],
	'banana': [ 105, 'banana', ],
	'walnut pieces': [ 190, 'quartercup', ],
	'peanuts': [ 161, 'oz', ],
	'peanut butter': [ 188, '2tbsp', ],
	'sardines': [ 36, 'oz', ],
	'tuna': [ 110, '3ozcan', ],
	'protein bar': [ 90, 'bar', ],
	'sushi': [ 33.3, 'roll', ],
	'hummus': [ 25, 'tbsp', ],
	'egg': [ 60, 'egg', ],
	'salami': [ 41, 'slice', ],
	'turkey': [ 54, 'oz', ],
	'jerky': [ 130, 'stick', ],
	'cereal with milk': [ 230, 'bowl', ],
	'rice cake': [ 35, 'rice cake', ],
	'seaweed': [ 48, 'serving', ],
	'knackebrot': [ 20, 'slice', ],
	'baked vegetable snack': [ 130, '1ozsrv', ],
	'ezekiel bread': [ 80, 'slice', ],
	'falafel': [ 57, 'falafel', ],
	'orzo salad': [ 332, 'cup', ],
	'feta olive mix': [ 336, '200g' ],
	'broccoli cheddar bowl': [ 460, 'bowl', ],
	'palak paneer': [ 410, 'meal', ],
	'mac and cheese': [ 450, 'meal' ],
	'chili': [ 540, '2srvcan' ],
	'cheesy scramble': [ 210, 'package', ],
	'impossible burger': [ 240, 'patty', ],
	'rockstar recovery': [ 10, 'can', ],
	'potato salad': [ 44, 'oz', ],
	'egg salad': [ 90, 'oz', ],
	'beef patty': [ 240, 'patty', ],
	'hamburger': [ 420, 'burger', ],
	'sausage and egg breakfast sandwich': [ 400, 'sandwich', ],
	'quiche': [ 420, 'serving', ],
	'orowheat bread': [ 100, 'slice', ],
	'sourdough bread': [ 120, 'slice', ],
	'whole rye german breads': [ 180, 'slice', ],
	'honey': [ 64, 'tbsp', ],
	'nutella': [ 100, 'tbsp', ],
	'camembert': [ 114, 'wedge', ],
	'brie': [ 110, 'oz', ],
	'blue cheese': [ 110, 'oz', ],
	'cheddar': [ 80, 'slice', ],
	'butter': [ 100, 'tbsp', ],
	'lasagna': [ 310, 'serving', ],
	'whiskey': [ 70, 'floz', ],
	'vodka': [ 64, 'floz', ],
	'morning pastry': [ 230, 'pastry', ],
	'pudding': [ 180, 'jar'],
	'cheetos': [ 160, 'oz', ],
	'candybar': [ 240, 'bar', ],
	'lollipop': [ 22, 'lollipop', ],
	'chips': [ 160, 'oz', ],
	'pizza with cheese': [ 250, 'slice', ],
	'gummi bears': [ 8, 'bear', ],
	'licorice pieces': [ 10, 'piece', ],
	'white chocolate': [ 160, 'oz', ],
}

lowcal_filling_nutritious = [
	'banana',
	'yogurt',
	'sardines',
	'protein bar',
	'egg',
	'falafel',
	'feta olive mix',
]

lowcal_snack = [
	'tomato grape',
	'raisins',
	'blueberries',
	'rice cake',
	'seaweed',
]

