import re
import urllib.request
import os

def readFileFromUrl(url):
    try:
        request = urllib.request.urlopen(url)
        result = request.read().decode('utf-8')
        return result
    except:
        print('Error getting file from url: ' + url + '\n')
        return ''

def getStringConsts(lang):
    print('Getting string consts for ' + lang, end='\n')
    langFile = readFileFromUrl('https://raw.githubusercontent.com/uTox/uTox/develop/langs/'+ lang + '.h')
    constsFinder = re.compile('msgid\((\w+)\)')
    consts = constsFinder.findall(langFile)
    return consts

def getMissingConsts(referenceConsts, translateArr):
    missingConsts = []
    for reference in referenceConsts:
        if reference not in translateArr:
            missingConsts.append(reference)
    return missingConsts

def writeReportForLangToFile(file, lang, consts):
    print('Writing report for ' + lang, end='\n')
    with open(file, "a") as report:
        report.write('---------------------------\nMissing translations for ' + lang + ':\n---------------------------\n')
        for const in consts:
            report.write(const + '\n')
        report.write('\n')

os.chdir(os.path.dirname(__file__))
reportFile = os.path.join(os.getcwd(), 'utox_missing_translations_report.txt')

langs = []
referenceLang = 'en'

langsStr = readFileFromUrl('https://raw.githubusercontent.com/uTox/uTox/develop/langs/i18n_decls.h')
finder = re.compile('\s+LANG_(\w{2}),')

referenceConsts = getStringConsts(referenceLang)

for match in finder.findall(langsStr):
    lang = match.lower()
    if lang != referenceLang:
        langConsts = getStringConsts(lang)
        missingConsts = getMissingConsts(referenceConsts, langConsts)
        writeReportForLangToFile(reportFile, lang, missingConsts)

print('Report generated')
