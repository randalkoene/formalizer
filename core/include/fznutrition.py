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
	'dried pineapple': [ 150, '1/3 cups', ],
	'celery': [ 7, 'stalk', ],
	'tomato grape': [ 1, 'grape', ],
	'grapes': [ 2, 'grape', ],
	'bell pepper': [ 33, 'large 150g', ],
	'apple': [ 52, 'apple', ],
	'orange': [ 45, 'orange', ],
	'mandarin': [ 47, 'mandarin', ],
	'blueberries': [ 85, 'cup', ],
	'avocado': [ 322, 'avocado', ],
	'banana': [ 105, 'banana', ],
	'apple sauce': [ 48, '100g', ],
	'canned peaches': [ 120, 'can', ],
	'walnut pieces': [ 190, 'quartercup', ],
	'pecans': [ 220, '30g', ],
	'peanuts': [ 161, 'oz', ],
	'cashews': [ 157, 'oz', ],
	'peanut butter': [ 188, '2tbsp', ],
	'tuna': [ 110, '3ozcan', ],
	'crab salad': [ 381, 'cup', ],
	'protein bar': [ 90, 'bar', ],
	'sushi': [ 33.3, 'roll', ],
	'hummus': [ 25, 'tbsp', ],
	'egg': [ 60, 'egg', ],
	'salami': [ 41, 'slice', ],
	'turkey': [ 54, 'oz', ],
	'jerky': [ 130, 'stick', ],
	'cereal with milk': [ 230, 'bowl', ],
	'muesli': [ 250, 'bowl', ],
	'rice cake': [ 35, 'rice cake', ],
	'seaweed': [ 48, 'serving', ],
	'knackebrot': [ 20, 'slice', ],
	'sesame knackebrot': [ 65, 'slice', ],
	'baked vegetable snack': [ 130, '1ozsrv', ],
	'ezekiel bread': [ 80, 'slice', ],
	'falafel': [ 57, 'falafel', ],
	'tofu egg salad sandwich': [ 175, 'sandwich', ],
	'orzo salad': [ 332, 'cup', ],
	'feta olive mix': [ 336, '200g' ],
	'broccoli cheddar bowl': [ 460, 'bowl', ],
	'pesto tortellini': [ 530, 'bowl', ],
	'palak paneer': [ 410, 'meal', ],
	'tofu miso soup with noodles': [ 370, 'bowl', ],
	'mac and cheese': [ 450, 'meal' ],
	'chili': [ 540, '2srvcan' ],
	'vegetarian burrito': [ 630, 'burrito', ],
	'bibimbap': [ 130, '162g serving', ],
	'cheesy scramble': [ 210, 'package', ],
	'impossible burger': [ 240, 'patty', ],
	'rockstar recovery': [ 10, 'can', ],
	'potato salad': [ 44, 'oz', ],
	'egg salad': [ 90, 'oz', ],
	'turkey': [ 189, '100g', ],
	'cranberry sauce': [ 418, 'cup', ],
	'stuffing': [ 386, '100g', ],
	'yams': [ 177, 'cup', ],
	'pumpkin pie': [ 323, 'slice', ],
	'beef patty': [ 240, 'patty', ],
	'hamburger': [ 420, 'burger', ],
	'double quarter pounder': [ 417, 'burger', ],
	'french fries': [ 378, 'medium serving', ],
	'hashbrown': [ 85, 'patty', ],
	'ham': [ 123, '3 oz slice', ],
	'omlette': [ 94, 'large', ],
	'sea bass': [ 124, '3oz serving', ],
	'sausage': [ 391, 'link (1/4 lb)', ],
	'fried breakfast potatoes': [ 157, 'cup', ],
	'sausage and egg breakfast sandwich': [ 400, 'sandwich', ],
	'falafel kebab': [ 437, 'serving', ],
	'doner kebab burrito': [ 505, 'kebab', ],
	'quiche': [ 420, 'serving', ],
	'orowheat bread': [ 100, 'slice', ],
	'sourdough bread': [ 120, 'slice', ],
	'sticky bun': [ 309, 'large', ],
	'bagel': [ 245, 'bagels', ],
	'whole rye german breads': [ 180, 'slice', ],
	'panini bread': [ 96, 'flat bun', ],
	'honey': [ 64, 'tbsp', ],
	'nutella': [ 100, 'tbsp', ],
	'camembert': [ 114, 'wedge', ],
	'brie': [ 110, 'oz', ],
	'blue cheese': [ 110, 'oz', ],
	'cheddar': [ 80, 'slice', ],
	'gouda': [ 337, '100g', ],
	'provolone': [ 98, 'slice', ],
	'cream cheese': [ 80, '2 tbsp', ],
	'butter': [ 100, 'tbsp', ],
	'lasagna': [ 310, 'serving', ],
	'orange juice': [ 86, 'glass', ],
	'cola': [ 138, 'can', ],
	'whiskey': [ 70, 'floz', ],
	'vodka': [ 64, 'floz', ],
	'jagermeister': [ 103, '30 ml'],
	'morning pastry': [ 230, 'pastry', ],
	'creme brule': [ 286, '260g serving', ],
	'pudding': [ 180, 'jar'],
	'cheetos': [ 160, 'oz', ],
	'stroopwafel': [ 186, 'waffle', ],
	'cheese cake': [ 257, 'piece', ],
	'apfel kuchen': [ 411, 'piece', ],
	'vegan white chocolate': [ 6, 'g', ],
	'mentos': [ 388, '100g', ],
	'candybar': [ 240, 'bar', ],
	'cookies': [ 502, '100g', ],
	'marzipan': [ 112, '25g', ],
	'lollipop': [ 22, 'lollipop', ],
	'banana chips': [ 200, '18 pieces', ],
	'chips': [ 160, 'oz', ],
	'pizza with cheese': [ 250, 'slice', ],
	'gummi bears': [ 8, 'bear', ],
	'jelly beans': [ 4, 'beans', ],
	'licorice pieces': [ 10, 'piece', ],
	'white chocolate': [ 160, 'oz', ],
	'tiramisu': [ 572, 'serving', ],
	'spinach ravioli': [ 220, '10 pieces', ],
	'beer': [ 154, '1 can', ],
	'grapefruit': [ 52, '1 fruit', ],
	'baked beans': [ 392, 'cup', ],
	'ice cream': [ 410, 'pint', ],
	'saltine': [ 12, 'cracker', ],
	'cashews': [ 7, 'cashew', ],
	'sardines': [ 191, '3.75 oz cans', ],
	'salsa verde': [ 11, '2 tbsp', ],
	'salsa bravas': [ 324, 'serving', ],
	'rum': [ 64, 'serving', ],
	'balini': [ 138, 'full balini', ],
	'pancake': [ 64, 'pancake', ],
	'hamburger bun': [ 120, 'calories', ],
	'corned beef': [ 120, 'serving', ],
	'larabar': [ 200, 'bar', ],
	'raspberries': [ 1, '1 raspberry',],
	'crab cakes': [ 220, 'crabcake', ],
	'chickpeas': [ 46, '1 tbsp', ],
	'mayonaise': [ 94, '1 tbsp', ],
	'cucumber': [ 45, 'cucumber',],
	'onion': [ 44, 'medium', ],
	'salmon': [ 412, 'half filet', ],
	'rice': [ 206, 'cup', ],
	'medium tortilla': [ 60, 'tortilla', ],
	'nutrigrain bar': [ 130, 'bar', ],
	'strawberries': [ 6, 'strawberry', ],
	'swiss cheese': [ 80, 'slice', ],
	'wine': [ 123, 'glass', ],
	'potato': [ 161, 'medium potato', ],
	'peas': [ 210, '3.5 cup can', ],
	'green beans': [ 60, '3.5 cup can', ],
	'egg white wraps': [ 25, 'wrap', ],
	'graham cracker': [ 59, 'large rectangular', ],
	'hot dog': [ 151, 'hot dog', ],
	'olives': [ 4, 'olive', ],
	'pinto beans': [ 200, 'can',],
	'cinnamon bun': [ 284, 'bun', ],
	'oats': [ 300, 'cup', ],
	'hash browns': [ 326, '100 g', ],
	'cellophane noodles': [ 492, 'cup', ],
	'ramen': [ 188, 'serving', ],
	'tomato soup': [ 74, 'cup', ],
	'granulated sugar': [ 16, 'tsp', ],
	'marshmallow': [ 23, 'marshmallow', ],
	'double cheeseburger': [ 437, 'sandwich', ],
	'milk shake': [ 350, "regular shake", ],
	'biscoff cookies': [ 150, "serving", ],
	'kroket': [ 131, 'kroket', ],
	'winegums': [ 11, 'winegum', ],
	'liverwurst': [ 29, 'serving', ],
	'bagette': [ 888, 'bagette', ],
	'energy drink': [ 113, 'can',],
	'vla': [ 133, 'serving',],
	'croissant': [ 231, 'croissant', ],
	'krentenbol': [ 169, 'krentenbol', ],
	'bacon': [ 43, 'piece', ],
	'jam': [ 46, 'tblsp', ],
	'light jam': [ 115, '100 g', ],
	'tuna salad': [ 50, 'tblsp', ],
	'borrelnoten': [ 131, 'serving', ],
	'german brotchen': [ 150, 'roll', ],
	'karnemelk': [ 29, '100 ml', ],
	'stokbrood flute': [ 807, 'whole', ],
	'boterhamworst': [ 251, '100g', ],
	'greek yogurt': [ 59, '100 g', ],
	'rookworst': [ 198, '100 g', ],
	'cherries': [ 77, '100 g', ],
	'saurkraut': [ 19, '100 g', ],
	'plums': [ 30, 'plum', ],
	'quarter pounder': [ 417, 'item', ],
	'fruit smoothie': [ 90, 'serving', ],
	'survival tabs': [ 240, 'tab', ],
	'corn tortilla chips': [ 293, 'cup', ],
}

lowcal_filling_nutritious = [
	'banana',
	'yogurt',
	'sardines',
	'protein bar',
	'egg',
	'falafel',
	'feta olive mix',
	'hummus',
	'greek yogurt',
	'saurkraut',
	'tuna',
]

lowcal_snack = [
	'tomato grape',
	'raisins',
	'blueberries',
	'rice cake',
	'seaweed',
	'cashews',
	'cherries',
	'pluma',
]

fruit = [
	'tomato grape',
	'grapes',
	'apple',
	'orange',
	'mandarin',
	'blueberries',
	'avocado',
	'banana',
	'dried apricots',
	'raisins',
	'dried figs',
	'dried pineapple',
	'apple sauce',
	'canned peaches',
	'cranberry sauce',
	'grapefruit',
	'raspberries',
	'strawberries',
	'olives',
	'cherries',
	'plums',
]

vegetables = [
	'celery',
	'bell pepper',
	'yams',
	'chickpeas',
	'cucumber',
	'onion',
	'potato',
	'fried breakfast potatoes',
	'hashbrown',
	'peas',
	'green beans',
	'pinto beans',
	'rice',
	'baked beans',
	'saurkraut',
]

breakfast = [
	'oats',
	'cereal with milk',
	'muesli',
	'morning pastry',
	'omlette',
	'pancake',
	'sticky bun',
	'sausage and egg breakfast sandwich',
	'cheesy scramble',
	'hash browns',
]

meals = [
	'protein shake',
	'orzo salad',
	'feta olive mix',
	'broccoli cheddar bowl',
	'pesto tortellini',
	'palak paneer',
	'tofu miso soup with noodles',
	'tofu egg salad sandwich',
	'mac and cheese',
	'chili',
	'vegetarian burrito',
	'impossible burger',
	'stuffing',	
	'hamburger',
	'double quarter pounder',
	'french fries',
	'falafel kebab',
	'doner kebab burrito',
	'quiche',
	'lasagna',
	'spinach ravioli',
	'salsa bravas',
	'pizza with cheese',
	'soylent',
	'balini',
	'bibimbap',
	'egg salad',
	'potato salad',
	'cellophane noodles',
	'ramen',
	'tomato soup',
	'double cheeseburger',
	'kroket',
	'quarter pounder',
]

drinks = [
	'coffee',
	'almond milk',
	'rockstar recovery',
	'orange juice',
	'cola',
	'whiskey',
	'vodka',
	'jagermeister',
	'beer',
	'rum',
	'wine',
	'energy drink',
	'karnemelk',
	'fruit smoothie',
]

breads = [
	'bagel',
	'ezekiel bread',
	'hamburger bun',
	'knackebrot',
	'medium tortilla',
	'orowheat bread',
	'panini bread',
	'saltine',
	'sourdough bread',
	'whole rye german breads',
	'egg white wraps',
	'bagette',
	'croissant',
	'krentenbol',
	'german brotchen',
	'stokbrood flute',
	'sesame knackebrot',
]

dairy = [
	'blue cheese',
	'brie',
	'camembert',
	'cheddar',
	'cream cheese',
	'gouda',
	'provolone',
	'swiss cheese',
]

seafood = [
	'crab cakes',
	'salmon',
	'sea bass',
	'sushi',
	'tuna',
	'crab salad',
	'tuna salad',
]

meat = [
	'beef patty',
	'corned beef',
	'ham',
	'jerky',
	'salami',
	'sausage',
	'turkey',
	'hot dog',
	'liverwurst',
	'bacon',
	'boterhamworst',
	'rookworst',
]

deserts = [
	'apfel kuchen',
	'cheese cake',
	'creme brule',
	'ice cream',
	'pudding',
	'pumpkin pie',
	'tiramisu',

]

nuts = [
	'peanuts',
	'pecans',
	'walnut pieces',
	'borrelnoten',
	'cashews',
]

spread = [
	'butter',
	'honey',
	'mayonaise',
	'nutella',
	'peanut butter',
	'jam',
	'liverwurst',
	'tuna salad',
	'light jam',
]

nutrition_groups = {
	'Low Calorie Nutritious': [ 'lowcalnutritious', lowcal_filling_nutritious ],
	'Low Calorie Snack': [ 'lowcalsnack', lowcal_snack ],
	'Fruit': [ 'fruit', fruit ],
	'Vegetables': [ 'vegetables', vegetables ],
	'Nuts': [ 'nuts', nuts ],
	'Dairy': [ 'dairy', dairy ],
	'Seafood': [ 'seafood', seafood ],
	'Meat': [ 'meat', meat ],
	'Spread': [ 'spread', spread ],
	'Breads': [ 'breads', breads ],
	'Deserts': [ 'deserts', deserts ],
	'Drinks': [ 'drinks', drinks ],
	'Breakfast': [ 'breakfast', breakfast ],
	'Meals': [ 'meals', meals ],
}
