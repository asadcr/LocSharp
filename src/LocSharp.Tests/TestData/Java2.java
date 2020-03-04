package com.triplemedia.util.csv;

/**
 * Utility class used to parse CSV files 
 */
public class CsvParser {
	/**
	 * Default fields separator value
	 */
	public static final char STD_VALUE_SEPERATOR = ',';
	
	/**
	 * Fields separator
	 */
	private char valueSeperator = ',';
	
	/**
	 * The minimum number of expected columns (fields)
	 */
	private int minColNumber = -1;
	
	/**
	 * Default constructor
	 * Uses STD_VALUE_SEPERATOR as fields separator and doesn't check the minimum number of columns
	 */
	public CsvParser(){		
	}

	/**
	 * Class constructor
	 * Uses STD_VALUE_SEPERATOR as fields separator
	 * @param minColNumber minimum number of expected columns (fields)
	 */
	public CsvParser(int minColNumber){
		this.minColNumber = minColNumber;
	}
	
	/**
	 * Class constructor
	 * Doesn't check the minimum number of columns
	 * @param valueSeparator fields separator
	 */
	public CsvParser(char valueSeparator){
		this.valueSeperator = valueSeparator;
	}
	
	/**
	 * Class constructor 
	 * @param valueSeparator fields separator
	 * @param minColNumber minimum number of expected columns (fields)
	 */
	public CsvParser(char valueSeparator, int minColNumber){
		this.valueSeperator = valueSeparator; 
		this.minColNumber = minColNumber;
	}	
	

	/**
	 * Extracts the values from a given string; removes leading and trailing whitespaces
	 * @param line the string to be parsed
	 * @return an array of strings containing fields found in the given string; an empty string
	 * is set for a missing column value
	 * @throws InvalidCsvColumnException if not found the minimum expected number of fields
	 */
	public String[] extractValuesFromLine(String line)throws InvalidCsvColumnException{
		String[] values = line.split(Character.toString(valueSeperator), -1);
		if ( minColNumber > 0 ){
			if ( values.length < minColNumber ){
				throw new InvalidCsvColumnException("Invalid number of columns found: " + values.length);
			}
		}
		for(int i = 0; i < values.length; i++){
			values[i] = values[i].trim();
			if(values[i].equals("")){
				continue;
			}
		}
		return values;		
	}
}
