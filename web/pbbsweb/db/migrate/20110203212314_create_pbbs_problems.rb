class CreatePbbsProblems < ActiveRecord::Migration
  def self.up
    create_table :pbbs_problems do |t|

      t.timestamps
    end
  end

  def self.down
    drop_table :pbbs_problems
  end
end
