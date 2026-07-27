# JSON/JSONL Parser
## Team
| Name:
|------|------|
| Javier Herrera Jr 
| Jules
| Ryan

---

## Lab Session Check-In (Monday, July 27)

**What option did you choose?**

Our group chose option 2, the JSON analysis

**What exactly will your software do?**

The software will try to parse through any JSON files while using the least RAM possible so that it's versitile for any file and any computer. The plan is to use parallels to try and accomplish this

**What language(s) will you use?**

We will only use C++

**What do you hope to accomplish in the remaining 2 hours?**

    - Lay out the 4 sprints
    - Describe feature sets
    - Work on sprint 1 which is a basic JSON parser with no performance

## JSON Examples

    - Sort
        Query: SORT(a, “asc”)

        JSON: [{"a": 1, "b": 2}, {"a": 0, "b": 3}]

        The answer should be [{"a": 0, "b": 3}, {"a": 1, "b": 2}]
    - Limit
        Query: LIMIT(1)

        JSON: [{"a": 1, "b": 2}, {"a": 0, "b": 3}]

        The answer should be {"a": 1, "b": 2}
    - Get
        Query: .a

        JSON: {“a”: 1, “b”: 2}

        The answer should be 1
    - GroupBy
        Query: GROUPBY(.a)

        JSON: [{"a": 1, "b": 2}, {"a": 0, "b": 3}, {"a": 1, "b": 5}]

        The answer should be: {
 	    "0": [{"a": 0, "b": 3}],
 	    "1": [{"a": 1, "b": 2}, {"a": 1, "b": 5}]
        }
    - Filter & Comparison
        Query: FILTER(.a > 1)

	    JSON: [{"a": 1, "b": 2}, {"a": 2, "b": 2}]

	    The answer should be [{"a": 2, "b": 2}]
    - Average
        Query: GROUPBY(.city) | AVERAGE(.price)

        JSON: [{"id": 1, "city": "Riverside", "price": 450000}, {"id": 2, "city": "Riverside", "price": 480000}, {"id": 3, "city": "Los Angeles", "price": 800000}, {"id": 4, "city": "Riverside", "price": 460000}]

        The answer should be {"Riverside": 463333.33, "Los Angeles": 800000}