# Brewing water

Our brewing water here in San Francisco and considerations for water additions.

## Lab Analysis

A water sample was taken on 10/15/2017 and sent to a well-known lab for analysis. Analysis was performed on 10/19/2017.

Location where the sample was taken: SOMA district in San Francisco (near 2nd and Bryant). This is an office building water supply and we're reasonably sure it's not been filtered.

The following values were reported directly from the laboratory. All values in ppm except pH.

| Value                    | Reported  |
| :----------------------: |:---------:|
| pH                       | 8.9       |
| TDS                      | 37        |
| Sodium (Na)              | 7         |
| Calcium (Ca)             | 4         | 
| Magnesium (Mg)           | <1        | 
| Total Hardness (CaCO3)   | 14        | 
| Sulfate (SO4)            | <1        |
| Chloride (Cl)            | 4         |
| Carbonate (CO3)          | <1        |
| Bicarbonate (HCO3)       | 14        |
| Total Alkalinity (CaCO3) | 13        |

It seems like there has been some concern about brewing water in SF when it became known that the Hetch Hetchy water would be [blended with local groundwater](https://sfwater.org/index.aspx?page=1136). This blending is intended to make the supply more stable and robust to droughts and emergencies. Fortunately our location in SOMA is not in the affected blending area (at least not yet). Even with the blended water supply, our water will likely always be very good for brewing, since there's really not too much in it to begin with.

Our lab analysis can be compared with the [2016 SF public water report](http://sfwater.org/index.aspx?page=634) published by the city:

| Value                    | Our water  | SF 2016 range | SF 2016 avg |
| :----------------------: |:---------: | :-----------: |:----------: |
| pH                       | 8.9        | 8.8-9.8       | 9.4         |
| TDS                      | 37         | <20-95        | 63          |
| Sodium (Na)              | 7          | 2.6-17        | 11          |
| Calcium (Ca)             | 4          | 2-18          | 10          |
| Magnesium (Mg)           | <1         | 0.2-          | 3.6         |
| Total Hardness (CaCO3)   | 14         | 8-76          | 44          |
| Sulfate (SO4)            | <1         | 1-30          | 16          |
| Chloride (Cl)            | 4          | 3-16          | 8.8         |
| Carbonate (CO3)          | <1         | N/A           | N/A         |
| Bicarbonate (HCO3)       | 14         | N/A           | N/A         |
| Total Alkalinity (CaCO3) | 13         | 7-112         | 39          |

So all of our values are within the reported range, and most of them are at the lower end.

## Brewing with SF water

Based on the laboratory water report, our water is very clean and does not require much modification for brewing. It's pretty low in Calcium, so we'll probably end up adding Calcium in the form of calcium chloride or gypsum (calcium sulfate), depending on the style of beer (more on the chloride side for maltier beers, and more sulfate for hoppier beers). We're targeting a Calcium level of 50-100 ppm for a healthy mash and fermentation.

Our water also has quite a high pH, which means we may need to add a bit more acid to bring our mash pH and sparge water pH to within acceptable ranges. But this isn't nearly as bad as high alkalinity, which neutralizes acid additions and makes pH adjustment more troublesome.

## Typical Additions

We use Calcium Chloride and Gypsum, which both provide Calcium ion.

We weigh our salt additions using a digital scale with 0.1 g resolution. It's better to measure by weight than by volume, since measuring by volume requires estimating the density of the salts, which is bound to be inaccurate. 

It's common to measure salt additions in units of grams of salt per gallon of brewing water, or g/gal.  To get to units of ppm (parts per million, which is the same as mg/L), a scale factor is used. This scale factor is different for the various types of salt.

One complicating factor is that salts often exist in dry form with attached [water of hydration](https://en.wikipedia.org/wiki/Water_of_crystallization), so the additional mass of the water molecules must be accounted for.

#### Gypsum (calcium sulfate dihydrate)

The molecular formula for calcium sulfate is CaSO4, yielding a molar mass of 136.1 g/mol in anhydrous form. It's most commonly available as the dihydrate salt known as gypsum, which has the molecular formula CaSO4•2H20, yielding a molar mass of 172.2 g/mol.

Gypsum is 23.2% calcium by mass (calcium's molecular weight is 40.0, which divided by 172.2 is 0.232). So 1 g of gypsum provides 0.232 g of Calcium. Dissolving 1 g of gypsum in 1 gallon of water (which is about 3785 g) results in a calcium concentration of 0.232/3785 = 6.14e-5 = 61.4 ppm.

The molar mass of sulfate is 96 g/mol, so by the same process we find that 1 g of gypsum in 1 gallon of water yields 147.4 ppm of sulfate.

For 1 gram dissolved in 1 gallon of water:

|                     | Calcium (ppm) | Sulfate (ppm) |
| :-----------------: |:-------------:|:-------------:|
| CaSO4•2H20 (gypsum) | 61.4          | 147.4         |

Gypsum, 1 gram in 1 gallon of water = 61.4 ppm Ca and 147.4 ppm SO4 (note that these are ions, but I'm omitting the charge notation).


#### Calcium Chloride

The molecular formula for calcium chloride is CaCl2, yielding a molar mass of 111 g/mol for the anhydrous form. Besides the anhydrous form (which is very hygroscopic and thus probably not a main constituent of the calcium chloride salts available for brewing), there exist monohydrate, dihydrate, tetrahydrate, and hexahydrate forms. It's not clear which version of the salt we are getting from brewing supply companies, and in reality it is probably a mix of a few of the salts. 

For 1 gram dissolved in 1 gallon of water:

|                | Calcium (ppm) | Chloride (ppm) |
| :------------: |:-------------:|:--------------:|
| CaCl2          | 95.4          | 168.8          |
| CaCl2•H20      | 82.1          | 145.2          |
| **CaCl2•2H20** | **72.0**      | **127.4**      |
| CaCl2•4H20     | 57.9          | 102.3          |
| CaCl2•6H20     | 48.3          | 85.5           |

We assume that we, on average, have the dihydrate version, so it is highlighted above.

#### Calculating additions

We target a ppm value for Calcium (within the 50-100 ppm range, usually 85 ppm) as well as a desired sulfate to chloride ratio. With these two constraints, required additions of the two salts can be computed. This assumes 1 gallon of water.

Total Calcium (ppm) = (Calcium from base water) + m_CaCl2 * 72.0 + m_gyp * 61.4
Total Sulfate (ppm) = (Sulfate from base water) +  m_gyp * 147.4
Total Chloride (ppm) = (Chloride from base water) + m_CaCl2 * 127.4

where m_CaCl2 and m_gyp indicate the mass in grams of calcium chloride and gypsum added, respectively.

R = Total Sulfate / Total Chloride

For malty beers, use R=0.5. For hoppy beers, use R=2 or even higher.

From these equations, it's possible to compute the required additions of the two salts.

## References:

1) [SF Annual Water Quality Report 2016](http://sfwater.org/index.aspx?page=634)
2) [San Francisco Groundwater Supply Project](https://sfwater.org/index.aspx?page=1136)
3) [Brewing Better Beer](https://www.amazon.com/Brewing-Better-Beer-Advanced-Homebrewers/dp/0937381985) (Gordon Strong)

