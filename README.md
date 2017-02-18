# halite
Source code of bot for Halite, an online AI competition organized by two sigma.

Ranked 9/1592 in the official final.

I first noticed Halite in Jan 2017 from a Linkedin animation of two bots combatting. After exploring the website halite.io a little bit, I find it intellectually engaging. I tried to follow some tutorials in the forum and began to submit bots, starting from the famous overkill bot.

Every weekend I spent some time to improve the bot, adding features to overkill bot little by little. The overkill bot doesn't have traffic control, so adding a map to record movements and strength can help redirect bots from overcap. To avoid overkill from opponents, it is also useful to rearrange directions of sites near the combat border.

In the last weekend of competition, I finally sit on to really think about the behavior. Through observation and test, I found that not attacking border between self and opponents has a big advantage on the opponents which don't implement this strategy. It is simply not engaging war with multiple oppenonts in early game. The source code is the one I submit for the finals. I hesitated for publishing this not well-designed code, and even thought about refactoring the code. Then I thought the time can be spent on something better and treat it as a learning experience.

Two things that I have learned:
1) Spend some time on the inital design will save time in the long run. After spending some time to refactor the code to class design, I give up because it need lot of time to tune parameters. It would be better if I have started that way.
2) Fancy algorithm doesn't necessarilly beat simple and well-tailored algorithm. Have something working is bettern than something might be better and hard to implement.
