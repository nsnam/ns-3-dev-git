# Markdownlint Rules
#
# The Markdownlint style is defined according to the following rules:
# https://github.com/markdownlint/markdownlint/blob/main/docs/creating_styles.md
#
# The rules are defined in the following page:
# https://github.com/markdownlint/markdownlint/blob/main/docs/RULES.md

all

# Excluded rules
exclude_rule "MD013" # Line length
exclude_rule "MD024" # Multiple headers with the same content

# Rule parameters
rule "MD007", :indent => 2 # Unordered list indentation
rule "MD026", :punctuation => ".,;:!" # Trailing punctuation in header
